#include "parser/parsers/proposal_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<ProposalRecord> ProposalParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    std::string tx_info;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        tx_info = ti.dump();  // 存储提案交易 txInfo
    } catch (...) {
        return {};  // 没有 data.txInfo 视为无效提案
    }
    if (tx_info.empty()) return {};

    ProposalRecord rec;
    rec.tx_hash = tx.hash;
    rec.address = addr;
    rec.tx_info = tx_info;
    // asset / is_first 由模块根据"是否已有 OHI 提案"决定
    return {rec};
}

}  // namespace hubsql
