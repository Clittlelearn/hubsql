#include "parser/parsers/contract_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int ContractModule::Process(sql::Connection& conn, const Transaction& tx,
                            uint64_t block_height) {
    int n = 0;
    for (auto& r : contract_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.Insert(conn, r);
        ++n;
    }
    return n;
}

nlohmann::json ContractModule::List(const nlohmann::json& filter, int page,
                                    int size) {
    const std::string addr = filter.value("address", "");
    const std::string tt   = filter.value("tx_type", "");
    const int flow_in      = filter.value("is_flow_in", -1);
    const int flow_out     = filter.value("is_flow_out", -1);
    auto result            = repo_.Query(addr, tt, flow_in, flow_out, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json ContractModule::Counts() {
    return {{"contract_records", repo_.Count()}};
}

}  // namespace hubsql
