#include "parser/parsers/revoke_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<RevokeRecord> RevokeParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    std::string proposal_hash;
    std::string tx_info;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("proposalHash")) proposal_hash = ti["proposalHash"].get<std::string>();
        tx_info = ti.dump();
    } catch (...) {
        return {};
    }
    if (proposal_hash.empty()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];

    RevokeRecord rec;
    rec.tx_hash       = tx.hash;       // 撤销交易 hash
    rec.address       = addr;
    rec.proposal_hash = proposal_hash; // 被撤销的提案（OHI 或提案hash）
    rec.tx_info       = tx_info;
    rec.time          = tx.time;       // 撤销时间(微秒)
    return {rec};
}

}  // namespace hubsql
