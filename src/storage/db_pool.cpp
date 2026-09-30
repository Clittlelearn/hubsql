#include "storage/db_pool.h"

#include <cppconn/exception.h>
#include <mysql_connection.h>
#include <mysql_driver.h>

#include <chrono>
#include <stdexcept>

#include "common/logger.h"

namespace hubsql {

DbPool::DbPool(const MysqlConfig& cfg) : cfg_(cfg) {
    if (cfg_.pool_size <= 0 || cfg_.pool_acquire_timeout_ms <= 0 ||
        cfg_.connect_timeout_seconds <= 0 || cfg_.read_timeout_seconds <= 0 ||
        cfg_.write_timeout_seconds <= 0 || cfg_.port <= 0 || cfg_.port > 65535) {
        throw std::invalid_argument("Invalid MySQL pool size, port or timeout");
    }
    idle_.reserve(cfg_.pool_size);
    connection_factory_ = [this] { return CreateConnection(); };
}

DbPool::~DbPool() = default;

std::unique_ptr<sql::Connection> DbPool::CreateConnection() {
    static sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    sql::ConnectOptionsMap options;
    options["hostName"] = cfg_.host;
    options["port"] = cfg_.port;
    options["userName"] = cfg_.user;
    options["password"] = cfg_.password;
    options["OPT_CONNECT_TIMEOUT"] = cfg_.connect_timeout_seconds;
    options["OPT_READ_TIMEOUT"] = cfg_.read_timeout_seconds;
    options["OPT_WRITE_TIMEOUT"] = cfg_.write_timeout_seconds;
    // Reconnecting within a transaction would silently lose transaction state.
    options["OPT_RECONNECT"] = false;
    std::unique_ptr<sql::Connection> conn(driver->connect(options));
    conn->setSchema(cfg_.database);
    return conn;
}

std::shared_ptr<sql::Connection> DbPool::Acquire() {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(cfg_.pool_acquire_timeout_ms);
    for (;;) {
        std::unique_ptr<sql::Connection> conn;
        bool reuse = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (std::chrono::steady_clock::now() >= deadline ||
                !cv_.wait_until(lock, deadline, [this] {
                    return !idle_.empty() || total_ < cfg_.pool_size;
                })) {
                throw std::runtime_error("Timed out waiting for a MySQL pool connection");
            }
            if (!idle_.empty()) {
                conn = std::move(idle_.back());
                idle_.pop_back();
                reuse = true;
            } else {
                ++total_;
            }
        }
        // Never hold the pool mutex during network operations.
        if (reuse) {
            bool valid = false;
            try {
                valid = !conn->isClosed() && conn->isValid();
            } catch (...) {
                // A disconnected idle connection is replaced before use.
            }
            if (!valid) {
                conn.reset();
                DropSlot();
                LOG_WARN("Discarded invalid idle MySQL connection");
                continue;
            }
        } else {
            try {
                conn = connection_factory_();
                if (!conn) throw std::runtime_error("MySQL connection factory returned null");
            } catch (...) {
                DropSlot();
                throw;
            }
        }
        return std::shared_ptr<sql::Connection>(conn.release(),
            [this](sql::Connection* c) noexcept { Release(c); });
    }
}

void DbPool::DropSlot() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --total_;
    }
    cv_.notify_one();
}

void DbPool::Release(sql::Connection* raw) noexcept {
    std::unique_ptr<sql::Connection> conn(raw);
    try {
        if (conn->isClosed() || !conn->isValid()) {
            throw std::runtime_error("Disconnected MySQL connection");
        }
        if (!conn->getAutoCommit()) {
            conn->rollback();
            conn->setAutoCommit(true);
        }
    } catch (...) {
        conn.reset();
        DropSlot();
        // The shared_ptr deleter must not throw while unwinding a SQL error.
        try { LOG_WARN("Discarded MySQL connection after validation/reset failure"); }
        catch (...) {}
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        idle_.push_back(std::move(conn));
    }
    cv_.notify_one();
}

}  // namespace hubsql
