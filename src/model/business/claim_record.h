#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 申领记录（claim_records 表，type=99 BONUS）
struct ClaimRecord {
    std::string tx_hash;         // 申领交易 hash
    uint64_t block_height{0};    // 申领所在区块高度
    std::string address;         // 申领地址
    std::string asset_type;      // 申领资产类型（hash 或 OHI）
    std::string amount;          // 申领金额（bonusAmount）
    uint64_t time{0};            // 申领时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
