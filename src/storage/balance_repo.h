#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "storage/db_pool.h"
#include "utxo/utxo_processor.h"
#include "model/transaction.h"

namespace hubsql {

// 单条余额
struct BalanceItem {
    std::string address;
    std::string asset_type;
    int64_t balance;
};
struct Erc20BalanceItem {
    std::string contract_address;
    std::string balance;  // uint256 十进制字符串
};
struct AssetCatalogItem {
    std::string kind;
    std::string asset_id;
    std::string name;
    std::string symbol;
    std::string contract_address;
    std::string asset_type;
    int decimals{8};
    bool is_added{false};
    bool is_flow_in{false};
};

// 三类余额表读写：OHI、提案 hash 资产、ERC20
class BalanceRepo {
public:
    explicit BalanceRepo(DbPool& pool);

    // 事务内批量应用余额增量（INSERT ... ON DUPLICATE KEY UPDATE balance += diff）
    void ApplyDeltas(const std::unordered_map<BalanceKey, int64_t, BalanceKeyHash>& deltas);

    // 单地址某资产余额；asset_type 为空则返回该地址所有资产中的第一个
    std::optional<int64_t> GetBalance(const std::string& addr,
                                      const std::string& asset_type = "");

    // 全部余额（按余额降序）；asset_type 为空 = 全部资产，否则只取该资产
    std::vector<BalanceItem> ListAll(const std::string& asset_type = "");

    // 某资产总余额（asset_type 为空 = 全部资产求和）
    int64_t TotalBalance(const std::string& asset_type = "");

    // 从已执行合约的 ERC20 Transfer 日志更新余额（调用者事务内）
    void ApplyErc20Transfers(sql::Connection& conn, const Transaction& tx,
                             const nlohmann::json& execution_result);

    bool AssociateErc20(const std::string& account, const std::string& contract);
    bool RemoveErc20(const std::string& account, const std::string& contract);
    std::vector<Erc20BalanceItem> ListErc20(const std::string& account);
    std::vector<AssetCatalogItem> ListAssetCatalog(const std::string& account);

private:
    DbPool& pool_;
};

}  // namespace hubsql
