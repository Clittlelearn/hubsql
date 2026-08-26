#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 投票记录（votes 表，type=13 VOTE）
struct VoteRecord {
    std::string tx_hash;          // 投票交易 hash
    uint64_t block_height{0};     // 投票所在区块高度
    std::string address;          // 投票人地址
    std::string proposal_hash;    // 被投票的提案hash（第一笔提案为 OHI）
    uint64_t proposal_type{0};    // 被投票交易类型（voteTxType，提案=11）
    uint64_t vote_type{0};        // 投票类型（0=反对 1=赞成）
    uint64_t vote_number{0};      // 票数

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
