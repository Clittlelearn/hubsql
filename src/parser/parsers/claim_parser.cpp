#include "parser/parsers/claim_parser.h"

#include <nlohmann/json.hpp>
#include "parser/parsers/parser_util.h"

namespace hubsql {

std::vector<ClaimRecord> ClaimParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    std::string claim_amount = "0";
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("bonusAmount")) {
            const auto& v = ti["bonusAmount"];
            claim_amount  = JsonRawAmount(v);
        }
    } catch (...) {
        return {};  // 没有 data.txInfo 视为无效申领
    }

    ClaimRecord rec;
    rec.tx_hash    = tx.hash;
    rec.address    = addr;
    rec.asset_type = tx.utxos[0].assetType;  // 申领资产类型（OHI 或 hash）
    rec.amount     = claim_amount;  // 8位原始申领金额
    rec.time       = tx.time;                       // 申领时间
    return {rec};
}

}  // namespace hubsql
