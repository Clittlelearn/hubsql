#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/fund_record.h"
#include "storage/db_pool.h"

namespace sql { class Connection; }

namespace hubsql {

struct FundQueryResult {
    std::vector<FundRecord> items;
    int64_t total{0};
};

class FundRepo {
public:
    explicit FundRepo(DbPool& pool) : pool_(pool) {}

    void Insert(sql::Connection& conn, const FundRecord& rec);
    FundQueryResult Query(const std::string& address,
                          const std::string& contract_address,
                          int page, int size);
    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql

