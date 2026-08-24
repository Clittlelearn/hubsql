#include "model/business/investment_record.h"

namespace hubsql {

nlohmann::json InvestmentRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]      = tx_hash;
    j["block_height"] = block_height;
    j["address"]      = address;
    j["amount"]       = amount;
    j["product_id"]   = product_id;
    return j;
}

}  // namespace hubsql
