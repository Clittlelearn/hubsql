#include "storage/vote_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

VoteRepo::VoteRepo(DbPool& pool) : pool_(pool) {}

void VoteRepo::Insert(sql::Connection& conn, const VoteRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO votes "
        "(tx_hash, block_height, address, proposal_hash, proposal_type, "
        " vote_type, vote_number) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setUInt64(2, rec.block_height);
    pstmt->setString(3, rec.address);
    pstmt->setString(4, rec.proposal_hash);
    pstmt->setUInt64(5, rec.proposal_type);
    pstmt->setUInt64(6, rec.vote_type);
    pstmt->setUInt64(7, rec.vote_number);
    pstmt->executeUpdate();
}

VoteQueryResult VoteRepo::Query(const std::string& address,
                                const std::string& proposal_hash,
                                int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> VoteQueryResult {
        VoteQueryResult result;
        const std::string ph_cond = proposal_hash.empty() ? "" : " AND proposal_hash = ?";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM votes "
                "WHERE (? = '' OR address = ?)" + ph_cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (!ph_cond.empty()) pstmt->setString(3, proposal_hash);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, block_height, address, proposal_hash, "
                "proposal_type, vote_type, vote_number FROM votes "
                "WHERE (? = '' OR address = ?)" + ph_cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (!ph_cond.empty()) pstmt->setString(idx++, proposal_hash);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                VoteRecord rec;
                rec.tx_hash       = ToStd(res->getString("tx_hash"));
                rec.block_height  = res->getUInt64("block_height");
                rec.address       = ToStd(res->getString("address"));
                rec.proposal_hash = ToStd(res->getString("proposal_hash"));
                rec.proposal_type = res->getUInt64("proposal_type");
                rec.vote_type     = res->getUInt64("vote_type");
                rec.vote_number   = res->getUInt64("vote_number");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t VoteRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM votes"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
