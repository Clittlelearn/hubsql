#include "parser/parsers/unlock_parser.h"

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<UnlockRecord> UnlockParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    std::string ref;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("unLockUtxo")) ref = ti["unLockUtxo"].get<std::string>();
    } catch (...) {
        return {};
    }
    if (ref.empty()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];

    UnlockRecord rec;
    rec.tx_hash      = tx.hash;   // 解锁定交易 hash
    rec.address      = addr;
    rec.lock_tx_hash = ref;       // 被解锁的锁定 tx hash
    rec.time         = tx.time;   // 解锁定时间(微秒)
    return {rec};
}

}  // namespace hubsql
