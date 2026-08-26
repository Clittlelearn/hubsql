#include "parser/parsers/lock_parser.h"

#include <cstdio>

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<LockRecord> LockParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    uint64_t lock_amount = 0;
    std::string lock_type;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("lockAmount")) {
            const auto& v = ti["lockAmount"];
            lock_amount   = v.is_number_unsigned() ? v.get<uint64_t>()
                           : v.is_number() ? static_cast<uint64_t>(v.get<double>()) : 0;
        }
        if (ti.contains("lockType")) lock_type = ti["lockType"].get<std::string>();
    } catch (...) {
        return {};  // 没有 data.txInfo 视为无效锁定
    }

    LockRecord rec;
    rec.tx_hash    = tx.hash;
    rec.address    = addr;
    rec.asset_type = tx.utxos[0].assetType;  // 锁定资产类型（OHI 或 hash）
    rec.amount     = std::to_string(lock_amount);  // 锁定金额
    rec.time       = tx.time;                      // 锁定时间
    rec.lock_type  = lock_type;                    // 锁定类型
    return {rec};
}

}  // namespace hubsql
