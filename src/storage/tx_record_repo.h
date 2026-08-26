#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/tx_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct TxRecordQueryResult {
    std::vector<TxRecord> items;
    int64_t total{0};
};

// 交易记录存储（滚动窗口，仅保留最新 20 个高度）
class TxRecordRepo {
public:
    explicit TxRecordRepo(DbPool& pool);

    // 插入一条交易记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const TxRecord& rec);

    // 清理低于 keep_min_height 的旧记录（在调用方事务内执行）
    void Prune(sql::Connection& conn, uint64_t keep_min_height);

    // 查询；tx_type 为空则不过滤
    TxRecordQueryResult Query(const std::string& tx_type, int page, int size);

    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
