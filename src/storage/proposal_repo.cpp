#include "storage/proposal_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
}  // namespace

ProposalRepo::ProposalRepo(DbPool& pool) : pool_(pool) {}

void ProposalRepo::Insert(sql::Connection& conn, const ProposalRecord& rec) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO proposals "
        "(tx_hash, asset, block_height, address, tx_info, vote_count, is_first) "
        "VALUES (?, ?, ?, ?, CAST(? AS JSON), ?, ?)"));
    pstmt->setString(1, rec.tx_hash);
    pstmt->setString(2, rec.asset);
    pstmt->setUInt64(3, rec.block_height);
    pstmt->setString(4, rec.address);
    pstmt->setString(5, rec.tx_info);
    pstmt->setUInt64(6, rec.vote_count);
    pstmt->setBoolean(7, rec.is_first);
    pstmt->executeUpdate();
}

void ProposalRepo::MarkRevoked(sql::Connection& conn, const std::string& asset,
                               const std::string& revoke_tx_hash,
                               uint64_t revoke_time) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE proposals "
        "SET is_revoked = 1, revoke_tx_hash = ?, revoke_time = ? "
        "WHERE asset = ?"));
    pstmt->setString(1, revoke_tx_hash);
    pstmt->setUInt64(2, revoke_time);
    pstmt->setString(3, asset);
    pstmt->executeUpdate();
}

void ProposalRepo::IncrementVoteCount(sql::Connection& conn,
                                      const std::string& asset,
                                      uint64_t vote_number) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE proposals SET vote_count = vote_count + ? WHERE asset = ?"));
    pstmt->setUInt64(1, vote_number);
    pstmt->setString(2, asset);
    pstmt->executeUpdate();
}

bool ProposalRepo::HasOhieProposal(sql::Connection& conn) {
    std::unique_ptr<sql::Statement> stmt(conn.createStatement());
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery("SELECT COUNT(*) FROM proposals WHERE asset = 'OHI'"));
    return res->next() && res->getInt64(1) > 0;
}

ProposalQueryResult ProposalRepo::Query(const std::string& address,
                                        int is_revoked, int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> ProposalQueryResult {
        ProposalQueryResult result;
        const std::string cond = is_revoked >= 0 ? " AND is_revoked = ?" : "";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM proposals "
                "WHERE (? = '' OR address = ?)" + cond));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            if (is_revoked >= 0) pstmt->setInt(3, is_revoked);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, asset, block_height, address, tx_info, "
                "vote_count, is_first, is_revoked, revoke_tx_hash, revoke_time "
                "FROM proposals "
                "WHERE (? = '' OR address = ?)" + cond +
                " ORDER BY block_height DESC LIMIT ? OFFSET ?"));
            pstmt->setString(1, address);
            pstmt->setString(2, address);
            int idx = 3;
            if (is_revoked >= 0) pstmt->setInt(idx++, is_revoked);
            pstmt->setInt(idx++, size);
            pstmt->setInt(idx, (page - 1) * size);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                ProposalRecord rec;
                rec.tx_hash        = ToStd(res->getString("tx_hash"));
                rec.asset          = ToStd(res->getString("asset"));
                rec.block_height   = res->getUInt64("block_height");
                rec.address        = ToStd(res->getString("address"));
                rec.tx_info        = ToStd(res->getString("tx_info"));
                rec.vote_count     = res->getUInt64("vote_count");
                rec.is_first       = res->getBoolean("is_first");
                rec.is_revoked     = res->getBoolean("is_revoked");
                rec.revoke_tx_hash = ToStd(res->getString("revoke_tx_hash"));
                rec.revoke_time    = res->getUInt64("revoke_time");
                result.items.push_back(std::move(rec));
            }
        }
        return result;
    });
}

int64_t ProposalRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SELECT COUNT(*) FROM proposals"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

int64_t ProposalRepo::CountRevoked() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT COUNT(*) FROM proposals WHERE is_revoked = 1"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
