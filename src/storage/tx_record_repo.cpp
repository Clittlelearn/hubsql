#include "storage/tx_record_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

TxRecordRepo::TxRecordRepo(DbPool& pool) : pool_(pool) {}

void TxRecordRepo::Insert(sql::Connection& conn, const TxRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO tx_records (tx_hash, block_height, tx_type, utxo_json) "
        "VALUES (?, ?, ?, CAST(? AS JSON))"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.tx_type);
    pstmt->setString(4, rec.utxo_json);
    pstmt->executeUpdate();
}

void TxRecordRepo::Prune(sql::Connection& conn, uint64_t keep_min_height) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "DELETE FROM tx_records WHERE block_height < ?"));
    pstmt->setUInt64(1, keep_min_height);
    pstmt->executeUpdate();
}

TxRecordQueryResult TxRecordRepo::Query(const std::string& tx_type,
                                        int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> TxRecordQueryResult {
        TxRecordQueryResult result;
        const std::string cond = tx_type.empty() ? "" : " WHERE tx_type = ?";

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM tx_records" + cond));
            if (!tx_type.empty()) pstmt->setString(1, tx_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, tx_type, utxo_json FROM tx_records" +
                cond + " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            int idx = 1;
            if (!tx_type.empty()) pstmt->setString(idx++, tx_type);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                TxRecord rec;
                rec.tx_hash      = ToStd(res->getString("tx_hash"));
                rec.block_height = res->getUInt64("block_height");
                rec.tx_type      = ToStd(res->getString("tx_type"));
                rec.utxo_json    = ToStd(res->getString("utxo_json"));
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t TxRecordRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM tx_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
