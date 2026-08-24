#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/transaction.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct TxQueryResult {
    std::vector<Transaction> items;
    int64_t total{0};
};

// 交易表读写
class TxRepo {
public:
    explicit TxRepo(DbPool& pool);

    // 幂等写入（事务内调用）
    void Insert(sql::Connection& conn, const Transaction& tx);

    // 便捷查询（可按类型/地址过滤，分页）
    TxQueryResult Query(const std::string& type, const std::string& address,
                        int page, int size);
    std::optional<Transaction> FindByHash(const std::string& tx_hash);
    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
