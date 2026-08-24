#pragma once

#include <cstdint>

#include <nlohmann/json.hpp>

#include "storage/block_repo.h"

namespace hubsql {

// 区块查询控制器（返回统一响应 JSON）
class BlockController {
public:
    explicit BlockController(BlockRepo& repo);

    nlohmann::json List(uint64_t start, uint64_t end, int page, int size);
    nlohmann::json Detail(uint64_t height);

private:
    BlockRepo& repo_;
};

}  // namespace hubsql
