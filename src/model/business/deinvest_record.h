#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 解投资记录（type=5 UNDELEGATE）：仅用于将对应投资记录标记为已解投资，不删除
struct DeinvestRecord {
    std::string tx_hash;          // 解投资交易 hash
    uint64_t block_height{0};     // 解投资所在区块高度
    std::string address;          // 解投资地址
    std::string invest_tx_hash;   // 被解的投资 tx hash（txInfo.undelegatingUtxo）
    uint64_t time{0};             // 解投资时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
