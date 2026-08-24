#include "storage/db_pool.h"

#include <cppconn/exception.h>
#include <mysql_connection.h>
#include <mysql_driver.h>

#include <stdexcept>

#include "common/logger.h"

namespace hubsql {

DbPool::DbPool(const MysqlConfig& cfg) : cfg_(cfg) {}

DbPool::~DbPool() = default;

std::unique_ptr<sql::Connection> DbPool::CreateConnection() {
    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    std::unique_ptr<sql::Connection> conn(
        driver->connect(cfg_.host, cfg_.user, cfg_.password));
    conn->setSchema(cfg_.database);
    return conn;
}

std::shared_ptr<sql::Connection> DbPool::Acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !idle_.empty() || total_ < cfg_.pool_size; });

    sql::Connection* raw = nullptr;
    if (!idle_.empty()) {
        raw = idle_.back().release();
        idle_.pop_back();
    } else {
        ++total_;
        raw = CreateConnection().release();
    }
    return std::shared_ptr<sql::Connection>(raw,
                                            [this](sql::Connection* c) { Release(c); });
}

void DbPool::Release(sql::Connection* conn) {
    // 复位连接状态：回滚残留事务 + 恢复自动提交，避免污染后续复用
    try {
        conn->rollback();
        conn->setAutoCommit(true);
    } catch (...) {
        // 忽略复位异常
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        idle_.emplace_back(conn);
    }
    cv_.notify_one();
}

}  // namespace hubsql
