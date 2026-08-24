#include "parser/parsers/staking_parser.h"

#include <cstdio>

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<StakeRecord> StakingParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    uint64_t stake_amount = 0;
    std::string commission = "0";
    std::string stake_type;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("stakeAmount")) {
            const auto& v = ti["stakeAmount"];
            stake_amount  = v.is_number_unsigned() ? v.get<uint64_t>()
                          : v.is_number() ? static_cast<uint64_t>(v.get<double>())
                          : 0;
        }
        if (ti.contains("commissionRate")) {
            const double c = ti["commissionRate"].get<double>();
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.6f", c);
            commission = buf;
        }
        if (ti.contains("stakeType")) stake_type = ti["stakeType"].get<std::string>();
    } catch (...) {
        // 没有 data.txInfo 视为无效质押
        return {};
    }

    StakeRecord rec;
    rec.tx_hash         = tx.hash;
    rec.address         = addr;
    rec.amount          = std::to_string(stake_amount);  // 质押金额
    rec.time            = tx.time;                       // 质押时间(微秒)
    rec.commission_rate = commission;                    // 佣金率
    rec.stake_type      = stake_type;                    // 质押类型
    return {rec};
}

}  // namespace hubsql
