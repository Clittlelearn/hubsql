#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/vote_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct VoteQueryResult {
    std::vector<VoteRecord> items;
    int64_t total{0};
};

// 投票业务存储（独立于其他业务）
class VoteRepo {
public:
    explicit VoteRepo(DbPool& pool);

    // 投票：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const VoteRecord& rec);

    // 查询；proposal_hash 为空则不过滤
    VoteQueryResult Query(const std::string& address,
                          const std::string& proposal_hash, int page, int size);

    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
