#include "parser/parsers/vote_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<VoteRecord> VoteParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    std::string vote_hash;
    uint64_t vote_tx_type = 0, vote_type = 0, vote_number = 0;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("voteHash")) vote_hash = ti["voteHash"].get<std::string>();
        if (ti.contains("voteTxType")) {
            const auto& v = ti["voteTxType"];
            vote_tx_type  = v.is_number_unsigned() ? v.get<uint64_t>()
                           : v.is_number() ? static_cast<uint64_t>(v.get<double>()) : 0;
        }
        if (ti.contains("voteType")) {
            const auto& v = ti["voteType"];
            vote_type     = v.is_number_unsigned() ? v.get<uint64_t>()
                           : v.is_number() ? static_cast<uint64_t>(v.get<double>()) : 0;
        }
        if (ti.contains("voteNumber")) {
            const auto& v = ti["voteNumber"];
            vote_number   = v.is_number_unsigned() ? v.get<uint64_t>()
                           : v.is_number() ? static_cast<uint64_t>(v.get<double>()) : 0;
        }
    } catch (...) {
        return {};
    }
    if (vote_hash.empty()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];

    VoteRecord rec;
    rec.tx_hash       = tx.hash;       // 投票交易 hash
    rec.address       = addr;
    rec.proposal_hash = vote_hash;     // 被投票的提案（OHI 或提案hash）
    rec.proposal_type = vote_tx_type;  // 被投票交易类型（提案=11）
    rec.vote_type     = vote_type;     // 投票类型（0=反对 1=赞成）
    rec.vote_number   = vote_number;   // 票数
    return {rec};
}

}  // namespace hubsql
