#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/business/proposal_record.h"
#include "storage/db_pool.h"

namespace sql {
class Connection;
}

namespace hubsql {

struct ProposalQueryResult {
    std::vector<ProposalRecord> items;
    int64_t total{0};
};

// 提案业务存储（含撤销标记与投票计数），独立于其他业务
class ProposalRepo {
public:
    explicit ProposalRepo(DbPool& pool);

    // 提案：插入一条记录（幂等，按 tx_hash/asset 去重；在调用方事务内执行）
    void Insert(sql::Connection& conn, const ProposalRecord& rec);

    // 撤销提案：仅标记（不删除）
    void MarkRevoked(sql::Connection& conn, const std::string& asset,
                     const std::string& revoke_tx_hash, uint64_t revoke_time);

    // 投票：累加提案投票数量
    void IncrementVoteCount(sql::Connection& conn, const std::string& asset,
                            uint64_t vote_number);

    // 是否已存在 OHI(第一笔) 提案（用于资产命名；在事务连接上查询，可见未提交写入）
    bool HasOhieProposal(sql::Connection& conn);

    // 查询；is_revoked: -1=全部, 0=未撤销, 1=已撤销
    ProposalQueryResult Query(const std::string& address, int is_revoked,
                              int page, int size);

    int64_t Count();
    int64_t CountRevoked();

private:
    DbPool& pool_;
};

}  // namespace hubsql
