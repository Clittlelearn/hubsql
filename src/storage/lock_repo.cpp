#include "storage/lock_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

LockRepo::LockRepo(DbPool& pool) : pool_(pool) {}

void LockRepo::Insert(sql::Connection& conn, const LockRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO lock_records "
        "(tx_hash, block_height, address, asset_type, lock_amount, lock_time, "
        " lock_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.asset_type);
    pstmt->setString(5, rec.amount);
    pstmt->setUInt64(6, rec.time);
    pstmt->setString(7, rec.lock_type);
    pstmt->executeUpdate();
}

void LockRepo::MarkUnlocked(sql::Connection& conn, const UnlockRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE lock_records "
        "SET is_unlocked = 1, unlock_tx_hash = ?, unlock_time = ? "
        "WHERE tx_hash = ?"));
    pstmt->setString(1, rec.tx_hash);      // 解锁定交易 hash
    pstmt->setUInt64(2, rec.time);         // 解锁定时间
    pstmt->setString(3, rec.lock_tx_hash); // 被解锁的锁定 tx hash
    pstmt->executeUpdate();
}

LockQueryResult LockRepo::Query(const std::string& address, int is_unlocked,
                                int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> LockQueryResult {
        LockQueryResult result;
        const std::string cond = is_unlocked >= 0 ? " AND is_unlocked = ?" : "";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM lock_records "
                "WHERE (? = '' OR address = ?)" + cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (is_unlocked >= 0) pstmt->setInt(3, is_unlocked);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, asset_type, lock_amount, "
                "lock_time, lock_type, is_unlocked, unlock_tx_hash, unlock_time "
                "FROM lock_records "
                "WHERE (? = '' OR address = ?)" + cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (is_unlocked >= 0) pstmt->setInt(idx++, is_unlocked);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                LockRecord rec;
                rec.tx_hash        = ToStd(res->getString("tx_hash"));
                rec.block_height   = res->getUInt64("block_height");
                rec.address        = ToStd(res->getString("address"));
                rec.asset_type     = ToStd(res->getString("asset_type"));
                rec.amount         = ToStd(res->getString("lock_amount"));
                rec.time           = res->getUInt64("lock_time");
                rec.lock_type      = ToStd(res->getString("lock_type"));
                rec.is_unlocked    = res->getBoolean("is_unlocked");
                rec.unlock_tx_hash = ToStd(res->getString("unlock_tx_hash"));
                rec.unlock_time    = res->getUInt64("unlock_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t LockRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM lock_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

int64_t LockRepo::CountUnlocked() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT COUNT(*) FROM lock_records WHERE is_unlocked = 1"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
