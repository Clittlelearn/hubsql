#include "parser/parsers/proposal_module.h"

#include <cppconn/connection.h>

#include "parser/parsers/proposal_common.h"

namespace hubsql {

int ProposalModule::Process(sql::Connection& conn, const Transaction& tx,
                            uint64_t block_height) {
    int n = 0;

    // PROPOSAL=11：插入提案
    for (auto& r : proposal_parser_.Parse(tx)) {
        r.block_height = block_height;
        // 资产命名：链上第一笔提案资产=OHI，之后=提案hash
        if (!repo_.HasOhieProposal(conn)) {
            r.asset    = "OHI";
            r.is_first = true;
        } else {
            r.asset    = tx.hash;
            r.is_first = false;
        }
        repo_.Insert(conn, r);
        ++n;
    }

    // REVOKEPROPOSAL=12：标记对应提案已撤销（不删除）
    for (auto& r : revoke_parser_.Parse(tx)) {
        r.block_height = block_height;
        const std::string asset = NormalizeProposalAsset(r.proposal_hash);
        repo_.ScheduleRevoke(conn, asset, r.tx_hash, r.tx_info, r.time);
        ++n;
    }
    return n;
}

void ProposalModule::OnBlockStart(sql::Connection& conn, uint64_t block_height,
                                  uint64_t block_time) {
    repo_.FinalizeNativeFlow(conn, block_height, block_time);
}

nlohmann::json ProposalModule::List(const nlohmann::json& filter, int page,
                                    int size) {
    const int rv           = filter.value("is_revoked", -1);  // -1=全部
    auto result            = repo_.Query(rv, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json ProposalModule::Counts() {
    return {{"proposals", repo_.Count()},
            {"revoked_proposals", repo_.CountRevoked()}};
}

}  // namespace hubsql
