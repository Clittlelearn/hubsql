#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 质押记录（staking_records 表，type=2 STAKE）
struct StakeRecord {
    std::string tx_hash;         // 质押交易 hash
    uint64_t block_height{0};    // 质押所在区块高度
    std::string address;         // 质押人地址
    std::string amount;          // 质押金额（字符串保精度）
    uint64_t time{0};            // 质押时间（微秒）
    std::string commission_rate; // 佣金率（commissionRate）
    std::string stake_type;      // 质押类型（stakeType，如 "Net"）

    // 解质押状态（查询展示；解质押仅标记不删除）
    bool is_unstaked{false};     // 是否已解质押
    std::string unstake_tx_hash; // 解质押交易 hash
    uint64_t unstake_time{0};    // 解质押时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
