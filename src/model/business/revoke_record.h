#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 撤销提案记录（type=12 REVOKEPROPOSAL）：仅用于将对应提案标记为已撤销，不删除
struct RevokeRecord {
    std::string tx_hash;          // 撤销交易 hash
    uint64_t block_height{0};     // 撤销所在区块高度
    std::string address;          // 撤销人地址
    std::string proposal_hash;    // 被撤销的提案（第一笔为 OHI，否则为提案hash）
    uint64_t time{0};             // 撤销时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
