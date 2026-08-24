#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "common/config.h"

namespace sql {
class Connection;
}

namespace hubsql {

// MySQL 连接池：线程安全，取用/归还
class DbPool {
public:
    explicit DbPool(const MysqlConfig& cfg);
    ~DbPool();

    DbPool(const DbPool&) = delete;
    DbPool& operator=(const DbPool&) = delete;

    // 获取一个连接（用完后自动归还池中）
    std::shared_ptr<sql::Connection> Acquire();

    // 回调式：WithConnection([](sql::Connection& conn){...})
    template <typename F>
    auto WithConnection(F&& f) -> decltype(f(std::declval<sql::Connection&>())) {
        auto conn = Acquire();
        return f(*conn);
    }

    int PoolSize() const { return cfg_.pool_size; }

private:
    void Release(sql::Connection* conn);
    std::unique_ptr<sql::Connection> CreateConnection();

    MysqlConfig cfg_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::unique_ptr<sql::Connection>> idle_;
    int total_{0};
};

}  // namespace hubsql
