#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/stake_record.h"
#include "model/business/unstake_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct StakingQueryResult {
    std::vector<StakeRecord> items;
    int64_t total{0};
};

// 质押业务存储（含解质押标记），独立于其他业务
class StakingRepo {
public:
    explicit StakingRepo(DbPool& pool);

    // 质押：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const StakeRecord& rec);

    // 解质押：将对应质押记录标记为已解质押（仅 UPDATE，不删除；在调用方事务内执行）
    void MarkUnstaked(sql::Connection& conn, const UnstakeRecord& rec);

    // 查询；is_unstaked: -1=全部, 0=未解质押, 1=已解质押
    StakingQueryResult Query(const std::string& address, int is_unstaked,
                             int page, int size);

    int64_t Count();
    int64_t CountUnstaked();

private:
    DbPool& pool_;
};

}  // namespace hubsql
