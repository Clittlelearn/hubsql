#include "storage/contract_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

ContractRepo::ContractRepo(DbPool& pool) : pool_(pool) {}

void ContractRepo::Insert(sql::Connection& conn, const ContractRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO contract_records "
        "(tx_hash, block_height, address, sender, recipient, tx_type, "
        " is_flow_in, is_flow_out, flow_in_amount, flow_out_amount, "
        " asset_type, tx_info, tx_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CAST(? AS JSON), ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.sender);
    pstmt->setString(5, rec.recipient);
    pstmt->setString(6, rec.tx_type);
    pstmt->setBoolean(7, rec.is_flow_in);
    pstmt->setBoolean(8, rec.is_flow_out);
    pstmt->setString(9, rec.flow_in_amount);
    pstmt->setString(10, rec.flow_out_amount);
    pstmt->setString(11, rec.asset_type);
    pstmt->setString(12, rec.tx_info);
    pstmt->setUInt64(13, rec.time);
    pstmt->executeUpdate();
}

ContractQueryResult ContractRepo::Query(const std::string& address,
                                        const std::string& tx_type,
                                        int is_flow_in, int is_flow_out,
                                        int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> ContractQueryResult {
        ContractQueryResult result;

        std::string cond = " WHERE (? = '' OR address = ?)";
        if (!tx_type.empty()) cond += " AND tx_type = ?";
        if (is_flow_in >= 0) cond += " AND is_flow_in = ?";
        if (is_flow_out >= 0) cond += " AND is_flow_out = ?";

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM contract_records" + cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (!tx_type.empty()) pstmt->setString(idx++, tx_type);
            if (is_flow_in >= 0) pstmt->setInt(idx++, is_flow_in);
            if (is_flow_out >= 0) pstmt->setInt(idx++, is_flow_out);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, sender, recipient, tx_type, "
                "is_flow_in, is_flow_out, flow_in_amount, flow_out_amount, "
                "asset_type, tx_info, tx_time FROM contract_records" + cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (!tx_type.empty()) pstmt->setString(idx++, tx_type);
            if (is_flow_in >= 0) pstmt->setInt(idx++, is_flow_in);
            if (is_flow_out >= 0) pstmt->setInt(idx++, is_flow_out);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                ContractRecord rec;
                rec.tx_hash         = ToStd(res->getString("tx_hash"));
                rec.block_height    = res->getUInt64("block_height");
                rec.address         = ToStd(res->getString("address"));
                rec.sender          = ToStd(res->getString("sender"));
                rec.recipient       = ToStd(res->getString("recipient"));
                rec.tx_type         = ToStd(res->getString("tx_type"));
                rec.is_flow_in      = res->getBoolean("is_flow_in");
                rec.is_flow_out     = res->getBoolean("is_flow_out");
                rec.flow_in_amount  = ToStd(res->getString("flow_in_amount"));
                rec.flow_out_amount = ToStd(res->getString("flow_out_amount"));
                rec.asset_type      = ToStd(res->getString("asset_type"));
                rec.tx_info         = ToStd(res->getString("tx_info"));
                rec.time            = res->getUInt64("tx_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t ContractRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM contract_records"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
