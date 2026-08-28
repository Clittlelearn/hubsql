#include "parser/parsers/fund_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int FundModule::Process(sql::Connection& conn, const Transaction& tx,
                        uint64_t block_height) {
    int count = 0;
    for (auto& rec : parser_.Parse(tx)) {
        rec.block_height = block_height;
        repo_.Insert(conn, rec);
        ++count;
    }
    return count;
}

nlohmann::json FundModule::List(const nlohmann::json& filter,
                                int page, int size) {
    auto result = repo_.Query(filter.value("address", ""),
                              filter.value("contract_address", ""),
                              page, size);
    nlohmann::json list = nlohmann::json::array();
    for (const auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json FundModule::Counts() {
    return {{"fund_records", repo_.Count()}};
}

}  // namespace hubsql

