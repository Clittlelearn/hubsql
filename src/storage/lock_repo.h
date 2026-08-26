#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/lock_record.h"
#include "model/business/unlock_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct LockQueryResult {
    std::vector<LockRecord> items;
    int64_t total{0};
};

// 锁定业务存储（含解锁定标记），独立于其他业务
class LockRepo {
public:
    explicit LockRepo(DbPool& pool);

    // 锁定：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const LockRecord& rec);

    // 解锁定：将对应锁定记录标记为已解锁定（仅 UPDATE，不删除；在调用方事务内执行）
    void MarkUnlocked(sql::Connection& conn, const UnlockRecord& rec);

    // 查询；is_unlocked: -1=全部, 0=未解锁定, 1=已解锁定
    LockQueryResult Query(const std::string& address, int is_unlocked,
                          int page, int size);

    int64_t Count();
    int64_t CountUnlocked();

private:
    DbPool& pool_;
};

}  // namespace hubsql
