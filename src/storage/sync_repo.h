#pragma once

#include <cstdint>
#include <string>

#include "storage/db_pool.h"

namespace hubsql {

// 同步进度读写（断点续拉）
class SyncRepo {
public:
    explicit SyncRepo(DbPool& pool);

    uint64_t GetLastSyncedHeight();
    void UpdateProgress(uint64_t height);

private:
    DbPool& pool_;
};

}  // namespace hubsql
