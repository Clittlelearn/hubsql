#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/deinvest_record.h"
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

// 投资业务存储（含解投资标记），独立于其他业务
class InvestmentRepo {
public:
    explicit InvestmentRepo(DbPool& pool);

    // 投资：插入一条记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const InvestmentRecord& rec);

    // 解投资：将对应投资记录标记为已解投资（仅 UPDATE，不删除；在调用方事务内执行）
    void MarkDeinvested(sql::Connection& conn, const DeinvestRecord& rec);

    // 查询；is_deinvested: -1=全部, 0=未解投资, 1=已解投资
    InvestmentQueryResult Query(const std::string& address, int is_deinvested,
                                int page, int size);

    int64_t Count();
    int64_t CountDeinvested();

private:
    DbPool& pool_;
};

}  // namespace hubsql
