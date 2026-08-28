#pragma once

#include "parser/ibusiness_module.h"
#include "parser/parsers/fund_parser.h"
#include "storage/fund_repo.h"

namespace hubsql {

class FundModule final : public IBusinessModule {
public:
    explicit FundModule(FundRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "fund"; }
    bool Handles(const Transaction& tx) const override { return tx.type == 8; }
    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;
    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    FundRepo& repo_;
    FundParser parser_;
};

}  // namespace hubsql

