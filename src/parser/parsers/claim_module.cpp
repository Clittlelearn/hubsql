#include "parser/parsers/claim_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int ClaimModule::Process(sql::Connection& conn, const Transaction& tx,
                         uint64_t block_height) {
    int n = 0;
    for (auto& r : claim_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.Insert(conn, r);
        ++n;
    }
    return n;
}

nlohmann::json ClaimModule::List(const nlohmann::json& filter, int page,
                                 int size) {
    const std::string addr = filter.value("address", "");
    const std::string at   = filter.value("asset_type", "");
    auto result            = repo_.Query(addr, at, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json ClaimModule::Counts() {
    return {{"claim_records", repo_.Count()}};
}

}  // namespace hubsql
