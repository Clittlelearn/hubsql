#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/contract_parser.h"
#include "storage/contract_repo.h"

namespace hubsql {

// 合约交易模块：解析 + 存储 一体，注册进 BusinessRegistry
// 处理 DEPLOY=7 / CALL=8（含跃入 FlowInTx / 跃出 FlowOutTx）
class ContractModule final : public IBusinessModule {
public:
    explicit ContractModule(ContractRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "contract"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 7 || tx.type == 8;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    ContractRepo& repo_;
    ContractParser contract_parser_;
};

}  // namespace hubsql
