#include "parser/parsers/invest_parser.h"

#include <cstdio>

#include <nlohmann/json.hpp>

namespace hubsql {

std::vector<InvestmentRecord> InvestParser::Parse(const Transaction& tx) {
    if (std::to_string(tx.type) != GetTxType()) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    uint64_t invest_amount = 0;
    std::string bonus_addr;
    std::string invest_type;
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        if (ti.contains("delegateAmount")) {
            const auto& v = ti["delegateAmount"];
            invest_amount = v.is_number_unsigned() ? v.get<uint64_t>()
                          : v.is_number() ? static_cast<uint64_t>(v.get<double>())
                          : 0;
        }
        if (ti.contains("bonusAddr")) bonus_addr = ti["bonusAddr"].get<std::string>();
        if (ti.contains("delegateType")) invest_type = ti["delegateType"].get<std::string>();
    } catch (...) {
        // 没有 data.txInfo 视为无效投资
        return {};
    }

    InvestmentRecord rec;
    rec.tx_hash     = tx.hash;
    rec.address     = addr;
    rec.amount      = std::to_string(invest_amount);  // 投资金额
    rec.time        = tx.time;                        // 投资时间(微秒)
    rec.bonus_addr  = bonus_addr;                     // bonusAddr
    rec.invest_type = invest_type;                    // 投资类型
    return {rec};
}

}  // namespace hubsql
