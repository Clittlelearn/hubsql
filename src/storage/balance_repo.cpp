#include "storage/balance_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

BalanceRepo::BalanceRepo(DbPool& pool) : pool_(pool) {}

void BalanceRepo::ApplyDeltas(const std::unordered_map<std::string, int64_t>& deltas) {
    if (deltas.empty()) return;

    pool_.WithConnection([&](sql::Connection& conn) {
        conn.setAutoCommit(false);
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "INSERT INTO account_balances (address, balance) VALUES (?, ?) "
            "ON DUPLICATE KEY UPDATE balance = balance + VALUES(balance)"));
        try {
            for (const auto& [addr, diff] : deltas) {
                pstmt->setString(1, addr);
                pstmt->setInt64(2, diff);
                pstmt->executeUpdate();
            }
            conn.commit();
        } catch (...) {
            conn.rollback();
            throw;
        }
        // 恢复自动提交，避免污染连接池中的连接
        conn.setAutoCommit(true);
    });
}

std::optional<int64_t> BalanceRepo::GetBalance(const std::string& addr) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::optional<int64_t> {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT balance FROM account_balances WHERE address = ?"));
        pstmt->setString(1, addr);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) return res->getInt64(1);
        return std::nullopt;
    });
}

int64_t BalanceRepo::TotalBalance() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COALESCE(SUM(balance),0) FROM account_balances"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

std::vector<std::pair<std::string, int64_t>> BalanceRepo::ListAll() {
    return pool_.WithConnection([](sql::Connection& conn) -> std::vector<std::pair<std::string, int64_t>> {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT address, balance FROM account_balances ORDER BY balance DESC"));
        std::vector<std::pair<std::string, int64_t>> out;
        while (res->next()) {
            out.emplace_back(std::string(res->getString("address").c_str()),
                             res->getInt64("balance"));
        }
        return out;
    });
}

}  // namespace hubsql
