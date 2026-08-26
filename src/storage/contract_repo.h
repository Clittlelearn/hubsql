#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/contract_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct ContractQueryResult {
    std::vector<ContractRecord> items;
    int64_t total{0};
};

// 合约交易存储（DEPLOY/CALL，含跃入跃出），独立于其他业务
class ContractRepo {
public:
    explicit ContractRepo(DbPool& pool);

    // 插入一条合约交易记录（幂等，按 tx_hash 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const ContractRecord& rec);

    // 查询；各过滤参数为空/负值表示不过滤
    ContractQueryResult Query(const std::string& address,
                              const std::string& tx_type,
                              int is_flow_in, int is_flow_out,
                              int page, int size);

    int64_t Count();

private:
    DbPool& pool_;
};

}  // namespace hubsql
