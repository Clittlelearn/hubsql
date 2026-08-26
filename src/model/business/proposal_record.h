#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 提案记录（proposals 表，type=11 PROPOSAL）
// 资产命名：第一笔提案的资产=OHI，之后提案的资产=提案hash
struct ProposalRecord {
    std::string tx_hash;         // 提案hash
    std::string asset;           // 资产名（OHI 或提案hash），用于投票/撤销关联
    uint64_t block_height{0};    // 提案所在区块高度
    std::string address;         // 提案人地址
    std::string tx_info;         // 提案交易 txInfo（JSON 字符串）
    uint64_t vote_count{0};      // 提案投票数量
    bool is_first{false};        // 是否第一笔提案（OHI 提案）

    // 撤销状态（查询展示；撤销仅标记不删除）
    bool is_revoked{false};      // 是否已撤销
    std::string revoke_tx_hash;  // 撤销提案交易 hash
    uint64_t revoke_time{0};     // 撤销时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
