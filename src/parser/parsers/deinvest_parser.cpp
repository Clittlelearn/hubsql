#include "parser/parsers/deinvest_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<DeinvestRecord> DeinvestParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    std::string ref;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("undelegatingUtxo")) ref = ti["undelegatingUtxo"].get<std::string>();
    } catch (...) {
        return {};
    }
    if (ref.empty()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];

    DeinvestRecord rec;
    rec.tx_hash        = tx.hash;      // 解投资交易 hash
    rec.address        = addr;
    rec.invest_tx_hash = ref;          // 被解的投资 tx hash
    rec.time           = tx.time;      // 解投资时间(微秒)
    return {rec};
}

}  // namespace hubsql
