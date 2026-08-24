#include "storage/investment_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

InvestmentRepo::InvestmentRepo(DbPool& pool) : pool_(pool) {}

void InvestmentRepo::Insert(sql::Connection& conn, const InvestmentRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO investment_records "
        "(tx_hash, block_height, address, amount, product_id) "
        "VALUES (?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.amount);
    pstmt->setString(5, rec.product_id);
    pstmt->executeUpdate();
}

InvestmentQueryResult InvestmentRepo::Query(const std::string& address,
                                            int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> InvestmentQueryResult {
        InvestmentQueryResult result;

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM investment_records "
                "WHERE (? = '' OR address = ?)"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, amount, product_id "
                "FROM investment_records "
                "WHERE (? = '' OR address = ?) "
                "ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            pstmt->setInt(3, size);
            pstmt->setInt(4, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                InvestmentRecord rec;
                rec.tx_hash      = ToStd(res->getString("tx_hash"));
                rec.block_height = res->getUInt64("block_height");
                rec.address      = ToStd(res->getString("address"));
                rec.amount       = ToStd(res->getString("amount"));
                rec.product_id   = ToStd(res->getString("product_id"));
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t InvestmentRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM investment_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
