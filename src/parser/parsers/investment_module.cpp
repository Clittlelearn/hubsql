#include "parser/parsers/investment_module.h"

#include <cppconn/connection.h>

namespace hubsql {

nlohmann::json InvestmentModule::List(const nlohmann::json& filter, int page,
                                      int size) {
    const std::string addr = filter.value("address", "");
    auto result            = repo_.Query(addr, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json InvestmentModule::Counts() {
    return {{"investment_records", repo_.Count()}};
}

}  // namespace hubsql
