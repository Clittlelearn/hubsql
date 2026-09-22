#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "storage/db_pool.h"
#include "utxo/utxo_processor.h"
#include "model/transaction.h"

namespace hubsql {

// 单条余额
struct BalanceItem {
    std::string address;
    std::string asset_type;
    std::string balance;  // 8 位原始整数，避免 uint64 溢出
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
    std::string native_flow_state;
    bool is_native_flow_active{false};
};

// 三类余额表读写：OHI、提案 hash 资产、ERC20
class BalanceRepo {
public:
    explicit BalanceRepo(DbPool& pool);

    // 事务内批量应用余额增量（INSERT ... ON DUPLICATE KEY UPDATE balance += diff）
    void ApplyDeltas(const std::unordered_map<BalanceKey, boost::multiprecision::cpp_int,
                                              BalanceKeyHash>& deltas);

    // 单地址某资产余额；asset_type 为空则返回该地址所有资产中的第一个
    std::optional<std::string> GetBalance(const std::string& addr,
                                          const std::string& asset_type = "");

    // 全部余额（按余额降序）；asset_type 为空 = 全部资产，否则只取该资产
    std::vector<BalanceItem> ListAll(const std::string& asset_type = "");

    // 某资产总余额（asset_type 为空 = 全部资产求和）
    std::string TotalBalance(const std::string& asset_type = "");

    // 优先从交易参数更新 ERC20 余额，无法判断时才回退到 EVM 日志（调用者事务内）
    void ApplyErc20Transfers(sql::Connection& conn, const Transaction& tx,
                             const nlohmann::json& execution_result);

    bool AssociateErc20(const std::string& account, const std::string& contract);
    bool RemoveErc20(const std::string& account, const std::string& contract);
    std::vector<Erc20BalanceItem> ListErc20(const std::string& account);
    // 返回解析区块得到的该账户全部 ERC20 余额，不要求用户先添加 Token。
    std::vector<Erc20BalanceItem> ListIndexedErc20(const std::string& account);
    std::vector<AssetCatalogItem> ListAssetCatalog(const std::string& account);

private:
    DbPool& pool_;
};

}  // namespace hubsql
