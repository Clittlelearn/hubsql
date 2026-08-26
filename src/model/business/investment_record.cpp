#include "model/business/investment_record.h"

namespace hubsql {

nlohmann::json InvestmentRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]         = tx_hash;
    j["block_height"]    = block_height;
    j["address"]         = address;
    j["invest_amount"]   = amount;
    j["invest_time"]     = time;
    j["bonus_addr"]      = bonus_addr;
    j["invest_type"]     = invest_type;
    j["is_deinvested"]   = is_deinvested;
    if (!deinvest_tx_hash.empty()) j["deinvest_tx_hash"] = deinvest_tx_hash;
    if (deinvest_time > 0) j["deinvest_time"] = deinvest_time;
    return j;
}

}  // namespace hubsql
