#include "storage/claim_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

ClaimRepo::ClaimRepo(DbPool& pool) : pool_(pool) {}

void ClaimRepo::Insert(sql::Connection& conn, const ClaimRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO claim_records "
        "(tx_hash, block_height, address, asset_type, claim_amount, claim_time) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.asset_type);
    pstmt->setString(5, rec.amount);
    pstmt->setUInt64(6, rec.time);
    pstmt->executeUpdate();
}

ClaimQueryResult ClaimRepo::Query(const std::string& address,
                                  const std::string& asset_type,
                                  int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> ClaimQueryResult {
        ClaimQueryResult result;
        const std::string cond = asset_type.empty() ? "" : " AND asset_type = ?";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM claim_records "
                "WHERE (? = '' OR address = ?)" + cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (!cond.empty()) pstmt->setString(3, asset_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, asset_type, "
                "claim_amount, claim_time FROM claim_records "
                "WHERE (? = '' OR address = ?)" + cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (!cond.empty()) pstmt->setString(idx++, asset_type);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                ClaimRecord rec;
                rec.tx_hash      = ToStd(res->getString("tx_hash"));
                rec.block_height = res->getUInt64("block_height");
                rec.address      = ToStd(res->getString("address"));
                rec.asset_type   = ToStd(res->getString("asset_type"));
                rec.amount       = ToStd(res->getString("claim_amount"));
                rec.time         = res->getUInt64("claim_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t ClaimRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM claim_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
