#include "parser/parsers/staking_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int StakingModule::Process(sql::Connection& conn, const Transaction& tx,
                           uint64_t block_height) {
    int n = 0;
    for (auto& r : staking_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.Insert(conn, r);
        ++n;
    }
    for (auto& r : unstaking_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.MarkUnstaked(conn, r);
        ++n;
    }
    return n;
}

nlohmann::json StakingModule::List(const nlohmann::json& filter, int page, int size) {
    const std::string addr = filter.value("address", "");
    const int iu           = filter.value("is_unstaked", -1);  // -1=全部
    auto result            = repo_.Query(addr, iu, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json StakingModule::Counts() {
    return {{"staking_records", repo_.Count()},
            {"unstaked_count", repo_.CountUnstaked()}};
}

}  // namespace hubsql
