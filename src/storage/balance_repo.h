#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "storage/db_pool.h"

namespace hubsql {

// 账户余额表（account_balances）读写
class BalanceRepo {
public:
    explicit BalanceRepo(DbPool& pool);

    // 事务内批量应用余额增量（INSERT ... ON DUPLICATE KEY UPDATE balance += diff）
    void ApplyDeltas(const std::unordered_map<std::string, int64_t>& deltas);

    std::optional<int64_t> GetBalance(const std::string& addr);
    int64_t TotalBalance();

    // 全部余额（按余额降序）
    std::vector<std::pair<std::string, int64_t>> ListAll();

private:
    DbPool& pool_;
};

}  // namespace hubsql
