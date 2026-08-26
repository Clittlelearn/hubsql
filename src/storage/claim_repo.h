#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/claim_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct ClaimQueryResult {
    std::vector<ClaimRecord> items;
    int64_t total{0};
};

// 申领业务存储（独立于其他业务）
class ClaimRepo {
public:
    explicit ClaimRepo(DbPool& pool);

    // 申领：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const ClaimRecord& rec);

    // 查询；asset_type 为空则不过滤
    ClaimQueryResult Query(const std::string& address,
                           const std::string& asset_type, int page, int size);

    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
