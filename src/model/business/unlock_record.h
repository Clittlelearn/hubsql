#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 解锁定记录（type=10 UNLOCK）：仅用于将对应锁定记录标记为已解锁定，不删除
struct UnlockRecord {
    std::string tx_hash;          // 解锁定交易 hash
    uint64_t block_height{0};     // 解锁定所在区块高度
    std::string address;          // 解锁定地址
    std::string lock_tx_hash;     // 被解锁的锁定 tx hash（txInfo.unLockUtxo）
    uint64_t time{0};             // 解锁定时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
