#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "storage/tx_record_repo.h"

namespace hubsql {

// 交易记录模块：记录全部交易（hash/type/utxo json），仅保留最新 20 个高度。
// 通过 OnBlockEnd 在每块结束后滚动清理旧记录。
class TxRecordModule final : public IBusinessModule {
public:
    // keep_heights: 保留的最新高度数量（默认 20）
    explicit TxRecordModule(TxRecordRepo& repo, uint64_t keep_heights = 20)
        : repo_(repo), keep_heights_(keep_heights) {}

    std::string Name() const override { return "txrecord"; }

    bool Handles(const Transaction&) const override { return true; }  // 全部交易

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    // 区块结束后清理低于最新 keep_heights 高度窗口的记录
    void OnBlockEnd(sql::Connection& conn, uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    TxRecordRepo& repo_;
    uint64_t keep_heights_;
};

}  // namespace hubsql
