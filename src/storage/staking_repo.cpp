#include "storage/staking_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

StakingRepo::StakingRepo(DbPool& pool) : pool_(pool) {}

void StakingRepo::Insert(sql::Connection& conn, const StakeRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO staking_records "
        "(tx_hash, block_height, address, stake_amount, stake_time, "
        " commission_rate, stake_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.amount);
    pstmt->setUInt64(5, rec.time);
    pstmt->setString(6, rec.commission_rate);
    pstmt->setString(7, rec.stake_type);
    pstmt->executeUpdate();
}

void StakingRepo::MarkUnstaked(sql::Connection& conn, const UnstakeRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE staking_records "
        "SET is_unstaked = 1, unstake_tx_hash = ?, unstake_time = ? "
        "WHERE tx_hash = ?"));
    pstmt->setString(1, rec.tx_hash);       // 解质押交易 hash
    pstmt->setUInt64(2, rec.time);          // 解质押时间
    pstmt->setString(3, rec.stake_tx_hash); // 被标记的质押 tx hash
    pstmt->executeUpdate();
}

StakingQueryResult StakingRepo::Query(const std::string& address,
                                      int is_unstaked, int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> StakingQueryResult {
        StakingQueryResult result;
        const std::string unst_cond =
            is_unstaked >= 0 ? " AND is_unstaked = ?" : "";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM staking_records "
                "WHERE (? = '' OR address = ?)" + unst_cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (is_unstaked >= 0) pstmt->setInt(3, is_unstaked);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, stake_amount, stake_time, "
                "commission_rate, stake_type, is_unstaked, unstake_tx_hash, "
                "unstake_time FROM staking_records "
                "WHERE (? = '' OR address = ?)" + unst_cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (is_unstaked >= 0) pstmt->setInt(idx++, is_unstaked);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                StakeRecord rec;
                rec.tx_hash          = ToStd(res->getString("tx_hash"));
                rec.block_height     = res->getUInt64("block_height");
                rec.address          = ToStd(res->getString("address"));
                rec.amount           = ToStd(res->getString("stake_amount"));
                rec.time             = res->getUInt64("stake_time");
                rec.commission_rate  = ToStd(res->getString("commission_rate"));
                rec.stake_type       = ToStd(res->getString("stake_type"));
                rec.is_unstaked      = res->getBoolean("is_unstaked");
                rec.unstake_tx_hash  = ToStd(res->getString("unstake_tx_hash"));
                rec.unstake_time     = res->getUInt64("unstake_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t StakingRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM staking_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

int64_t StakingRepo::CountUnstaked() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT COUNT(*) FROM staking_records WHERE is_unstaked = 1"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
