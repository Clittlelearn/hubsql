#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/investment_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct InvestmentQueryResult {
    std::vector<InvestmentRecord> items;
    int64_t total{0};
};

// 投资业务存储（独立于其他业务）
class InvestmentRepo {
public:
    explicit InvestmentRepo(DbPool& pool);

    // 投资：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const InvestmentRecord& rec);

    InvestmentQueryResult Query(const std::string& address, int page, int size);

    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
