#pragma once

#include <memory>

#include "common/config.h"
#include "parser/business_registry.h"
#include "storage/balance_repo.h"
#include "storage/block_repo.h"
#include "storage/db_pool.h"
#include "storage/tx_repo.h"

namespace hubsql {

// REST API 服务（crow 实现，pimpl 隔离）
class ApiServer {
public:
    ApiServer(const ApiConfig& cfg, DbPool& pool,
              BlockRepo& blocks, TxRepo& txs, BusinessRegistry& registry,
              BalanceRepo& balances);
    ~ApiServer();

    ApiServer(const ApiServer&) = delete;
    ApiServer& operator=(const ApiServer&) = delete;

    void Run();   // 阻塞式启动
    void Stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace hubsql
