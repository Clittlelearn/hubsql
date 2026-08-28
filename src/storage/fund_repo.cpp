#include "storage/fund_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {
namespace { std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); } }

void FundRepo::Insert(sql::Connection& conn, const FundRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO fund_records "
        "(tx_hash, block_height, sender, recipient, contract_address, fund_amount, fund_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.sender);
    pstmt->setString(4, rec.recipient);
    pstmt->setString(5, rec.contract_address);
    pstmt->setString(6, rec.amount);
    pstmt->setUInt64(7, rec.time);
    pstmt->executeUpdate();
}

FundQueryResult FundRepo::Query(const std::string& address,
                                const std::string& contract_address,
                                int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        FundQueryResult result;
        const std::string where =
            " WHERE (? = '' OR sender = ? OR recipient = ?)"
            " AND (? = '' OR contract_address = ?)";
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM fund_records" + where));
            pstmt->setString(1, address); pstmt->setString(2, address);
            pstmt->setString(3, address); pstmt->setString(4, contract_address);
            pstmt->setString(5, contract_address);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, sender, recipient, contract_address, "
                "fund_amount, fund_time FROM fund_records" + where +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address); pstmt->setString(2, address);
            pstmt->setString(3, address); pstmt->setString(4, contract_address);
            pstmt->setString(5, contract_address); pstmt->setInt(6, size);
            pstmt->setInt(7, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                FundRecord rec;
                rec.tx_hash = ToStd(res->getString("tx_hash"));
                rec.block_height = res->getUInt64("block_height");
                rec.sender = ToStd(res->getString("sender"));
                rec.recipient = ToStd(res->getString("recipient"));
                rec.contract_address = ToStd(res->getString("contract_address"));
                rec.amount = ToStd(res->getString("fund_amount"));
                rec.time = res->getUInt64("fund_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t FundRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM fund_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql

