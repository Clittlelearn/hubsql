#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "model/block.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

// 区块表读写
class BlockRepo {
public:
    explicit BlockRepo(DbPool& pool);

    // 幂等写入（事务内调用，由调用方管理连接）
    void Insert(sql::Connection& conn, const Block& block);

    // 以下为便捷查询（内部自动取/还连接）
    std::optional<Block> FindByHeight(uint64_t height);
    std::vector<Block> List(uint64_t start, uint64_t end, int page, int size);
    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
