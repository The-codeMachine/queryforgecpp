#include "QueryPool.h"

int main() {
    QueryPool pool(4, "tcp://127.0.0.1:3306", "user", "password", "database");

    auto future = pool.enqueue("SELECT * FROM users WHERE id = ?", {"42"});
    auto result = future.get();

    while (result && result->next()) {
        std::cout << "User: " << result->getString("username") << std::endl;
    }
}
