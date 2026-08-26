#include "parser/parsers/investment_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int InvestmentModule::Process(sql::Connection& conn, const Transaction& tx,
                              uint64_t block_height) {
    int n = 0;
    for (auto& r : invest_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.Insert(conn, r);
        ++n;
    }
    for (auto& r : deinvest_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.MarkDeinvested(conn, r);
        ++n;
    }
    return n;
}

nlohmann::json InvestmentModule::List(const nlohmann::json& filter, int page,
                                      int size) {
    const std::string addr = filter.value("address", "");
    const int di           = filter.value("is_deinvested", -1);  // -1=全部
    auto result            = repo_.Query(addr, di, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json InvestmentModule::Counts() {
    return {{"investment_records", repo_.Count()},
            {"deinvested_count", repo_.CountDeinvested()}};
}

}  // namespace hubsql
