#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 锁定记录（lock_records 表，type=9 LOCK）
struct LockRecord {
    std::string tx_hash;         // 锁定交易 hash
    uint64_t block_height{0};    // 锁定所在区块高度
    std::string address;         // 锁定地址
    std::string asset_type;      // 锁定资产类型（hash 或 OHI）
    std::string amount;          // 锁定金额（lockAmount）
    uint64_t time{0};            // 锁定时间（微秒）
    std::string lock_type;       // 锁定类型（lockType，如 LockNet）

    // 解锁定状态（查询展示；解锁定仅标记不删除）
    bool is_unlocked{false};     // 是否已解锁定
    std::string unlock_tx_hash;  // 解锁定交易 hash
    uint64_t unlock_time{0};     // 解锁定时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
