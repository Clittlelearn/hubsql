#include "storage/proposal_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <nlohmann/json.hpp>

#include "common/logger.h"

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }

std::string NormalizeVoteRef(std::string value) {
    if (value.size() > 2 && value[0] == '0' &&
        (value[1] == 'x' || value[1] == 'X')) {
        value.erase(0, 2);
    }
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ReadUint64(const nlohmann::json& value, const char* key, uint64_t& out) {
    const auto it = value.find(key);
    if (it == value.end()) return false;
    try {
        if (it->is_number_unsigned()) {
            out = it->get<uint64_t>();
            return true;
        }
        if (it->is_number_integer()) {
            const auto number = it->get<int64_t>();
            if (number < 0) return false;
            out = static_cast<uint64_t>(number);
            return true;
        }
        if (it->is_string()) {
            const std::string text = it->get<std::string>();
            std::size_t consumed = 0;
            out = std::stoull(text, &consumed);
            return consumed == text.size();
        }
    } catch (...) {
    }
    return false;
}

struct VoteStats {
    uint64_t approve{0};
    uint64_t reject{0};
    uint64_t voters{0};
};

VoteStats ReadVoteStats(sql::Connection& conn, const std::string& reference) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "SELECT COALESCE(SUM(CASE WHEN vote_type=1 THEN vote_number ELSE 0 END),0),"
        "COALESCE(SUM(CASE WHEN vote_type=0 THEN vote_number ELSE 0 END),0),"
        "COUNT(DISTINCT LOWER(address)) FROM votes WHERE "
        "LOWER(CASE WHEN LEFT(proposal_hash,2) IN ('0x','0X') "
        "THEN SUBSTRING(proposal_hash,3) ELSE proposal_hash END)=?"));
    pstmt->setString(1, NormalizeVoteRef(reference));
    std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());
    VoteStats stats;
    if (result->next()) {
        stats.approve = result->getUInt64(1);
        stats.reject = result->getUInt64(2);
        stats.voters = result->getUInt64(3);
    }
    return stats;
}

bool PassedAtBlock(const nlohmann::json& info, const std::string& vote_ref,
                   uint64_t block_time, sql::Connection& conn,
                   bool& ready) {
    uint64_t end_time = 0;
    uint64_t expiration = 0;
    uint64_t minimum_voters = 0;
    ready = ReadUint64(info, "endTime", end_time) && end_time != 0 &&
            block_time > end_time;
    if (!ready) return false;
    if (!ReadUint64(info, "expirationDate", expiration)) {
        expiration = std::numeric_limits<uint64_t>::max();
    }
    if (!ReadUint64(info, "minVoteNum", minimum_voters)) return false;
    const auto votes = ReadVoteStats(conn, vote_ref);
    return expiration >= end_time && block_time <= expiration &&
           votes.voters >= minimum_voters && votes.approve > votes.reject;
}
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

void ProposalRepo::ScheduleRevoke(sql::Connection& conn,
                                  const std::string& asset,
                                  const std::string& revoke_tx_hash,
                                  const std::string& revoke_tx_info,
                                  uint64_t revoke_time) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "UPDATE proposals "
        "SET revoke_tx_hash = ?, revoke_tx_info = CAST(? AS JSON), "
        "revoke_time = ?, revoke_state = 'pending' "
        "WHERE asset = ?"));
    pstmt->setString(1, revoke_tx_hash);
    pstmt->setString(2, revoke_tx_info);
    pstmt->setUInt64(3, revoke_time);
    pstmt->setString(4, asset);
    pstmt->executeUpdate();
}

void ProposalRepo::FinalizeNativeFlow(sql::Connection& conn,
                                      uint64_t block_height,
                                      uint64_t block_time) {
    struct Pending {
        std::string asset;
        std::string tx_info;
        std::string state;
        std::string revoke_hash;
        std::string revoke_info;
        std::string revoke_state;
    };
    std::vector<Pending> pending;
    {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(
            "SELECT asset,tx_info,native_flow_state,COALESCE(revoke_tx_hash,''),"
            "COALESCE(revoke_tx_info,JSON_OBJECT()),revoke_state FROM proposals "
            "WHERE native_flow_state='pending' OR "
            "(native_flow_state='active' AND revoke_state='pending') FOR UPDATE"));
        while (result->next()) {
            pending.push_back({ToStd(result->getString(1)),
                               ToStd(result->getString(2)),
                               ToStd(result->getString(3)),
                               ToStd(result->getString(4)),
                               ToStd(result->getString(5)),
                               ToStd(result->getString(6))});
        }
    }

    for (const auto& item : pending) {
        if (item.state == "pending") {
            std::string next_state;
            bool ready = false;
            try {
                const auto info = nlohmann::json::parse(item.tx_info);
                uint64_t cross_chain_type = 0;
                if (!ReadUint64(info, "crossChainTxType", cross_chain_type) ||
                    (cross_chain_type != 0 && cross_chain_type != 2)) {
                    next_state = "ineligible";
                    ready = true;
                } else {
                    next_state = PassedAtBlock(info, item.asset, block_time,
                                               conn, ready)
                                     ? "active"
                                     : "missing";
                }
            } catch (...) {
                next_state = "missing";
                ready = true;
            }
            if (ready) {
                std::unique_ptr<sql::PreparedStatement> update(conn.prepareStatement(
                    "UPDATE proposals SET native_flow_state=?,finalized_height=?,"
                    "finalized_time=? WHERE asset=? AND native_flow_state='pending'"));
                update->setString(1, next_state);
                update->setUInt64(2, block_height);
                update->setUInt64(3, block_time);
                update->setString(4, item.asset);
                update->executeUpdate();
                LOG_INFO("Native Flow proposal finalized asset={} state={} height={}",
                         item.asset, next_state, block_height);
            }
        }

        if (item.state == "active" && item.revoke_state == "pending" &&
            !item.revoke_hash.empty()) {
            bool ready = false;
            bool passed = false;
            try {
                passed = PassedAtBlock(nlohmann::json::parse(item.revoke_info),
                                       item.revoke_hash, block_time, conn, ready);
            } catch (...) {
                ready = true;
            }
            if (!ready) continue;
            std::unique_ptr<sql::PreparedStatement> update(conn.prepareStatement(
                "UPDATE proposals SET revoke_state=?,is_revoked=?,"
                "native_flow_state=IF(?=1,'revoked',native_flow_state),"
                "finalized_height=?,finalized_time=? WHERE asset=?"));
            update->setString(1, passed ? "revoked" : "missing");
            update->setBoolean(2, passed);
            update->setBoolean(3, passed);
            update->setUInt64(4, block_height);
            update->setUInt64(5, block_time);
            update->setString(6, item.asset);
            update->executeUpdate();
            LOG_INFO("Native Flow revoke finalized asset={} state={} height={}",
                     item.asset, passed ? "revoked" : "missing", block_height);
        }
    }
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

ProposalQueryResult ProposalRepo::Query(int is_revoked, int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> ProposalQueryResult {
        ProposalQueryResult result;
        const std::string cond = is_revoked >= 0 ? " AND is_revoked = ?" : "";

        // 总条数
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT COUNT(*) FROM proposals WHERE 1=1" + cond));
            if (is_revoked >= 0) pstmt->setInt(1, is_revoked);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            result.total = res->next() ? res->getInt64(1) : 0;
        }

        // 分页数据
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                "SELECT tx_hash, asset, block_height, address, tx_info, "
                "vote_count, is_first, is_revoked, revoke_tx_hash, revoke_time "
                "FROM proposals WHERE 1=1" + cond +
                " ORDER BY CAST(COALESCE(JSON_UNQUOTE(JSON_EXTRACT(tx_info,'$.beginTime')),'0') AS UNSIGNED) DESC, "
                "block_height DESC LIMIT ? OFFSET ?"));
            int idx = 1;
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
