#include "parser/parsers/lock_module.h"

#include <cppconn/connection.h>

namespace hubsql {

int LockModule::Process(sql::Connection& conn, const Transaction& tx,
                        uint64_t block_height) {
    int n = 0;
    for (auto& r : lock_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.Insert(conn, r);
        ++n;
    }
    for (auto& r : unlock_parser_.Parse(tx)) {
        r.block_height = block_height;
        repo_.MarkUnlocked(conn, r);
        ++n;
    }
    return n;
}

nlohmann::json LockModule::List(const nlohmann::json& filter, int page, int size) {
    const std::string addr = filter.value("address", "");
    const int ul           = filter.value("is_unlocked", -1);  // -1=全部
    auto result            = repo_.Query(addr, ul, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json LockModule::Counts() {
    return {{"lock_records", repo_.Count()},
            {"unlocked_count", repo_.CountUnlocked()}};
}

}  // namespace hubsql
