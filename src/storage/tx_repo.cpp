#include "storage/tx_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <stdexcept>

#include "utils/reflect_struct.h"

namespace hubsql {

namespace {
// SQLString -> std::string
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

TxRepo::TxRepo(DbPool& pool) : pool_(pool) {}

void TxRepo::Insert(sql::Connection& conn, const Transaction& tx) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO transactions "
        "(tx_hash, block_height, type, from_addr, to_addr, amount, raw_json) "
        "VALUES (?, 0, ?, '', '', '', CAST(? AS JSON))"));
    pstmt->setString(1, tx.hash);
    pstmt->setString(2, std::to_string(tx.type));
    pstmt->setString(3, tx.ToJson().dump());
    pstmt->executeUpdate();
}

TxQueryResult TxRepo::Query(const std::string& type, const std::string& address,
                            int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> TxQueryResult {
        TxQueryResult result;

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM transactions "
                "WHERE (? = '' OR type = ?) AND (? = '' OR from_addr = ?)"));
            pstmt->setString(1, type);
            pstmt->setString(2, type);
            pstmt->setString(3, address);
            pstmt->setString(4, address);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, type, raw_json "
                "FROM transactions "
                "WHERE (? = '' OR type = ?) AND (? = '' OR from_addr = ?) "
                "ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, type);
            pstmt->setString(2, type);
            pstmt->setString(3, address);
            pstmt->setString(4, address);
            pstmt->setInt(5, size);
            pstmt->setInt(6, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                Transaction tx;
                tx.hash = ToStd(res->getString("tx_hash"));
                try {
                    auto j = nlohmann::json::parse(ToStd(res->getString("raw_json")));
                    reflect::Deserialize(j, tx);  // 还原完整结构
                } catch (...) {
                    // 保留已填充字段
                }
                result.items.push_back(std::move(tx));
            }
        }
        return result;
    });
}

std::optional<Transaction> TxRepo::FindByHash(const std::string& tx_hash) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::optional<Transaction> {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT tx_hash, block_height, type, raw_json "
            "FROM transactions WHERE tx_hash = ?"));
        pstmt->setString(1, tx_hash);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) return std::nullopt;
        Transaction tx;
        tx.hash = ToStd(res->getString("tx_hash"));
        try {
            auto j = nlohmann::json::parse(ToStd(res->getString("raw_json")));
            reflect::Deserialize(j, tx);
        } catch (...) {
            // 保留已填充字段
        }
        return tx;
    });
}

int64_t TxRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT COUNT(*) FROM transactions"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
