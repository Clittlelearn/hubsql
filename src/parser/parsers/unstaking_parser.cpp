#include "parser/parsers/unstaking_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<UnstakeRecord> UnstakingParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    std::string ref;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("unstakeUtxo")) ref = ti["unstakeUtxo"].get<std::string>();
    } catch (...) {
        return {};
    }
    if (ref.empty()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];

    UnstakeRecord rec;
    rec.tx_hash       = tx.hash;      // 解质押交易 hash
    rec.address       = addr;
    rec.stake_tx_hash = ref;          // 被标记的质押 tx hash
    rec.time          = tx.time;      // 解质押时间(微秒)
    return {rec};
}

}  // namespace hubsql
