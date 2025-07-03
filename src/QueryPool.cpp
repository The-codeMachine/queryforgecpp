#include <include/QueryPool.h>
#include <include/ILogger.h>  // Note capitalization

QueryPool::QueryPool(int worker_count, const std::string& host,
                     const std::string& user, const std::string& password,
                     const std::string& database)
    : host(host), user(user), password(password), database(database) {
    driver = get_driver_instance();

    for (int i = 0; i < worker_count; ++i) {
        workers.emplace_back(&QueryPool::worker_loop, this);
    }
}

QueryPool::~QueryPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

std::future<std::unique_ptr<sql::ResultSet>> QueryPool::enqueue(const std::string& sql, const std::vector<std::string>& params) {
    auto* task = new QueryTask(sql, params);
    std::future<std::unique_ptr<sql::ResultSet>> res = task->result_promise.get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    condition.notify_one();

    return res;
}

void QueryPool::internal_worker_loop() {
    std::unique_ptr<sql::Connection> conn;
    std::unordered_map<std::string, std::shared_ptr<sql::PreparedStatement>> stmt_cache;

    auto connect = [&]() -> bool {
        try {
            // Build connection string with host
            std::string url = host;  // Example: "tcp://127.0.0.1:3306"

            conn = std::unique_ptr<sql::Connection>(driver->connect(url, user, password));
            conn->setSchema(database);
            return true;
        }
        catch (const sql::SQLException& e) {
            std::cerr << "[QueryPool] Failed to connect to DB: " << e.what() << std::endl;
            return false;
        }
    };

    while (!connect()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    while (true) {
        std::unique_ptr<QueryTask> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) return;
            task = std::unique_ptr<QueryTask>(tasks.front());
            tasks.pop();
        }

        bool handled = false;
        int retries = 0;
        const int max_retries = 3;

        while (!handled && retries < max_retries) {
            try {
                if (!conn || !conn->isValid()) {
                    std::cerr << "[QueryPool] Connection invalid before prepareStatement. Reconnecting..." << std::endl;
                    while (!connect()) std::this_thread::sleep_for(std::chrono::seconds(2));
                    stmt_cache.clear();
                }

                std::shared_ptr<sql::PreparedStatement> pstmt;
                if (stmt_cache.count(task->sql)) {
                    pstmt = stmt_cache[task->sql];
                } else {
                    pstmt = std::shared_ptr<sql::PreparedStatement>(conn->prepareStatement(task->sql));
                    stmt_cache[task->sql] = pstmt;
                }

                for (size_t i = 0; i < task->params.size(); ++i) {
                    pstmt->setString(static_cast<unsigned int>(i + 1), task->params[i]);
                }

                if (task->sql.find("SELECT") == 0) {
                    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
                    task->result_promise.set_value(std::move(res));
                } else {
                    pstmt->execute();
                    task->result_promise.set_value(nullptr);
                }

                handled = true;
            }
            catch (const sql::SQLException& e) {
                Logger::getInstance().log(LogLevel::ERR,
                    "SQL Exception: " + std::string(e.what()) + 
                    " (retry " + std::to_string(retries + 1) + "/" + std::to_string(max_retries) + ")");

                if (e.getErrorCode() == 2006 || e.getErrorCode() == 2013 || !conn->isValid()) {
                    Logger::getInstance().log(LogLevel::ERR, "[QueryPool] Lost connection. Attempting reconnect...");
                    if (!connect()) {
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    }
                } else {
                    task->result_promise.set_exception(std::current_exception());
                    handled = true;
                }

                ++retries;
            }
            catch (...) {
                task->result_promise.set_exception(std::current_exception());
                handled = true;
            }
        }

        if (!handled) {
            task->result_promise.set_exception(std::make_exception_ptr(std::runtime_error("Query failed after retries")));
        }
    }
}

void QueryPool::worker_loop() {
    while (!stop) {
        try {
            internal_worker_loop();
        }
        catch (const std::exception& e) {
            std::cerr << "[QueryPool] Worker crashed: " << e.what() << ". Restarting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        catch (...) {
            std::cerr << "[QueryPool] Worker crashed with unknown exception. Restarting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
