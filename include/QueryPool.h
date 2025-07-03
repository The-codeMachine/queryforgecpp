#pragma once

#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <unordered_map>

#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>

class QueryTask {
public:
    std::string sql;
    std::vector<std::string> params;
    std::promise<std::unique_ptr<sql::ResultSet>> result_promise;

    QueryTask(const std::string& query, const std::vector<std::string>& parameters)
        : sql(query), params(parameters) {}
};

class QueryPool {
public:
    QueryPool(int worker_count, const std::string& host, const std::string& user,
              const std::string& password, const std::string& database);
    ~QueryPool();

    std::future<std::unique_ptr<sql::ResultSet>> enqueue(const std::string& sql,
                                                         const std::vector<std::string>& params);

private:
    std::vector<std::thread> workers;
    std::queue<QueryTask*> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;

    sql::Driver* driver = nullptr;

    std::string host;
    std::string user;
    std::string password;
    std::string database;

    void worker_loop();
    void internal_worker_loop();
};
