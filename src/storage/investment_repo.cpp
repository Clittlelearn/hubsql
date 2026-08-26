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
        "(tx_hash, block_height, address, invest_amount, invest_time, "
        " bonus_addr, invest_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.amount);
    pstmt->setUInt64(5, rec.time);
    pstmt->setString(6, rec.bonus_addr);
    pstmt->setString(7, rec.invest_type);
    pstmt->executeUpdate();
}

void InvestmentRepo::MarkDeinvested(sql::Connection& conn,
                                    const DeinvestRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE investment_records "
        "SET is_deinvested = 1, deinvest_tx_hash = ?, deinvest_time = ? "
        "WHERE tx_hash = ?"));
    pstmt->setString(1, rec.tx_hash);       // 解投资交易 hash
    pstmt->setUInt64(2, rec.time);          // 解投资时间
    pstmt->setString(3, rec.invest_tx_hash); // 被解的投资 tx hash
    pstmt->executeUpdate();
}

InvestmentQueryResult InvestmentRepo::Query(const std::string& address,
                                            int is_deinvested,
                                            int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> InvestmentQueryResult {
        InvestmentQueryResult result;
        const std::string cond =
            is_deinvested >= 0 ? " AND is_deinvested = ?" : "";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM investment_records "
                "WHERE (? = '' OR address = ?)" + cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (is_deinvested >= 0) pstmt->setInt(3, is_deinvested);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, invest_amount, invest_time, "
                "bonus_addr, invest_type, is_deinvested, deinvest_tx_hash, "
                "deinvest_time FROM investment_records "
                "WHERE (? = '' OR address = ?)" + cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (is_deinvested >= 0) pstmt->setInt(idx++, is_deinvested);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                InvestmentRecord rec;
                rec.tx_hash          = ToStd(res->getString("tx_hash"));
                rec.block_height     = res->getUInt64("block_height");
                rec.address          = ToStd(res->getString("address"));
                rec.amount           = ToStd(res->getString("invest_amount"));
                rec.time             = res->getUInt64("invest_time");
                rec.bonus_addr       = ToStd(res->getString("bonus_addr"));
                rec.invest_type      = ToStd(res->getString("invest_type"));
                rec.is_deinvested    = res->getBoolean("is_deinvested");
                rec.deinvest_tx_hash = ToStd(res->getString("deinvest_tx_hash"));
                rec.deinvest_time    = res->getUInt64("deinvest_time");
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

int64_t InvestmentRepo::CountDeinvested() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT COUNT(*) FROM investment_records WHERE is_deinvested = 1"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
