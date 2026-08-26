#include "parser/parsers/vote_module.h"

#include <cppconn/connection.h>

#include "parser/parsers/proposal_common.h"

namespace hubsql {

int VoteModule::Process(sql::Connection& conn, const Transaction& tx,
                        uint64_t block_height) {
    int n = 0;
    for (auto& r : vote_parser_.Parse(tx)) {
        r.block_height = block_height;
        votes_.Insert(conn, r);
        ++n;
        // 累加对应提案的投票数量（资产：第一笔=OHI，之后=提案hash）
        const std::string asset = NormalizeProposalAsset(r.proposal_hash);
        proposals_.IncrementVoteCount(conn, asset, r.vote_number);
    }
    return n;
}

nlohmann::json VoteModule::List(const nlohmann::json& filter, int page,
                                int size) {
    const std::string addr = filter.value("address", "");
    const std::string ph   = filter.value("proposal_hash", "");
    auto result            = votes_.Query(addr, ph, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json VoteModule::Counts() {
    return {{"votes", votes_.Count()}};
}

}  // namespace hubsql
