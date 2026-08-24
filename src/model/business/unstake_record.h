#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 解质押记录（type=3 UNSTAKE）：仅用于将对应质押记录标记为已解质押，不删除
struct UnstakeRecord {
    std::string tx_hash;         // 解质押交易 hash
    uint64_t block_height{0};    // 解质押所在区块高度
    std::string address;         // 解质押人地址
    std::string stake_tx_hash;   // 被标记的质押 tx hash（txInfo.unstakeUtxo）
    uint64_t time{0};            // 解质押时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
