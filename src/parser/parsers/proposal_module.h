#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/proposal_parser.h"
#include "parser/parsers/revoke_parser.h"
#include "storage/proposal_repo.h"

namespace hubsql {

// 提案业务模块：解析 + 存储 一体，注册进 BusinessRegistry
// 处理 PROPOSAL=11（插入提案，资产命名：第一笔=OHI、之后=提案hash）、
//      REVOKEPROPOSAL=12（仅标记 is_revoked，不删除）
class ProposalModule final : public IBusinessModule {
public:
    explicit ProposalModule(ProposalRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "proposal"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 11 || tx.type == 12;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    void OnBlockStart(sql::Connection& conn, uint64_t block_height,
                      uint64_t block_time) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    ProposalRepo& repo_;
    ProposalParser proposal_parser_;
    RevokeParser revoke_parser_;
};

}  // namespace hubsql
