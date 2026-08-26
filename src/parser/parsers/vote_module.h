#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/vote_parser.h"
#include "storage/proposal_repo.h"
#include "storage/vote_repo.h"

namespace hubsql {

// 投票业务模块：解析 + 存储 一体，注册进 BusinessRegistry
// 处理 VOTE=13（插入投票 + 累加对应提案的投票数量）
// 与提案高度相关：投票通过 voteHash（"0xOHI" 或 "0x<提案hash>"）关联提案资产。
class VoteModule final : public IBusinessModule {
public:
    VoteModule(VoteRepo& votes, ProposalRepo& proposals)
        : votes_(votes), proposals_(proposals) {}

    std::string Name() const override { return "vote"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 13;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    VoteRepo& votes_;
    ProposalRepo& proposals_;
    VoteParser vote_parser_;
};

}  // namespace hubsql
