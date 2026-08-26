#include "storage/balance_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <cctype>

namespace hubsql {

namespace {
std::string ToStd(const sql::SQLString& s) { return std::string(s.c_str()); }
std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
std::string TopicAddress(const std::string& topic) {
    if (topic.size() < 40) return {};
    return "0x" + Lower(topic.substr(topic.size() - 40));
}
std::string HexToDec(const std::string& hex) {
    boost::multiprecision::cpp_int n = 0;
    for (char c : hex) {
        if (c == 'x' || c == 'X') { n = 0; continue; }
        int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (v >= 0) n = n * 16 + v;
    }
    return n.convert_to<std::string>();
}
}  // namespace

BalanceRepo::BalanceRepo(DbPool& pool) : pool_(pool) {}

void BalanceRepo::ApplyDeltas(
    const std::unordered_map<BalanceKey, int64_t, BalanceKeyHash>& deltas) {
    if (deltas.empty()) return;

    pool_.WithConnection([&](sql::Connection& conn) {
        conn.setAutoCommit(false);
        try {
            for (const auto& [key, diff] : deltas) {
                const bool ohi = key.asset_type == "OHI";
                std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                    ohi ? "INSERT INTO ohi_balances (address,balance) VALUES (?,?) ON DUPLICATE KEY UPDATE balance=balance+VALUES(balance)"
                        : "INSERT INTO proposal_asset_balances (address,asset_type,balance) VALUES (?,?,?) ON DUPLICATE KEY UPDATE balance=balance+VALUES(balance)"));
                pstmt->setString(1, key.address);
                if (ohi) pstmt->setInt64(2, diff);
                else { pstmt->setString(2, key.asset_type); pstmt->setInt64(3, diff); }
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

std::optional<int64_t> BalanceRepo::GetBalance(const std::string& addr,
                                               const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::optional<int64_t> {
        if (!asset_type.empty()) {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                asset_type == "OHI"
                    ? "SELECT balance FROM ohi_balances WHERE address = ?"
                    : "SELECT balance FROM proposal_asset_balances WHERE address = ? AND asset_type = ?"));
            pstmt->setString(1, addr);
            if (asset_type != "OHI") pstmt->setString(2, asset_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) return res->getInt64(1);
            return std::nullopt;
        }
        // 未指定资产：返回该地址任意一条
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT balance FROM ohi_balances WHERE address = ? LIMIT 1"));
        pstmt->setString(1, addr);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) return res->getInt64(1);
        return std::nullopt;
    });
}

std::vector<BalanceItem> BalanceRepo::ListAll(const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::vector<BalanceItem> {
        std::vector<BalanceItem> out;
        if (!asset_type.empty()) {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                asset_type == "OHI"
                  ? "SELECT address, 'OHI' asset_type, balance FROM ohi_balances ORDER BY balance DESC"
                  : "SELECT address, asset_type, balance FROM proposal_asset_balances WHERE asset_type=? ORDER BY balance DESC"));
            if (asset_type != "OHI") pstmt->setString(1, asset_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                out.push_back({ToStd(res->getString("address")),
                               ToStd(res->getString("asset_type")),
                               res->getInt64("balance")});
            }
        } else {
            std::unique_ptr<sql::Statement> stmt(conn.createStatement());
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
                "SELECT address, asset_type, balance FROM (SELECT address,'OHI' asset_type,balance FROM ohi_balances UNION ALL SELECT address,asset_type,balance FROM proposal_asset_balances) b ORDER BY balance DESC"));
            while (res->next()) {
                out.push_back({ToStd(res->getString("address")),
                               ToStd(res->getString("asset_type")),
                               res->getInt64("balance")});
            }
        }
        return out;
    });
}

int64_t BalanceRepo::TotalBalance(const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            asset_type.empty()
              ? "SELECT COALESCE(SUM(balance),0) FROM (SELECT balance FROM ohi_balances UNION ALL SELECT balance FROM proposal_asset_balances) b"
              : asset_type == "OHI" ? "SELECT COALESCE(SUM(balance),0) FROM ohi_balances"
              : "SELECT COALESCE(SUM(balance),0) FROM proposal_asset_balances WHERE asset_type=?"));
        if (!asset_type.empty() && asset_type != "OHI") pstmt->setString(1, asset_type);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next() ? res->getInt64(1) : 0;
    });
}

void BalanceRepo::ApplyErc20Transfers(sql::Connection& conn, const Transaction& tx,
                                      const nlohmann::json& execution_result) {
    if (tx.type != 7 && tx.type != 8) return;
    if (!execution_result.is_object()) return;
    nlohmann::json info;
    try { info = nlohmann::json::parse(tx.data).value("txInfo", nlohmann::json::object()); }
    catch (...) { return; }
    const auto logs = execution_result.value("log", nlohmann::json::array());
    static const std::string transfer_topic = "ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef";
    const std::string zero = "0x0000000000000000000000000000000000000000";
    uint32_t log_index = 0;
    for (const auto& log : logs) {
        const uint32_t current_index = log_index++;
        auto topics = log.value("topics", nlohmann::json::array());
        if (topics.size() < 3 || Lower(topics[0].get<std::string>()) != transfer_topic) continue;
        std::string contract = TopicAddress(log.value("creator", ""));
        std::string from = TopicAddress(topics[1].get<std::string>());
        std::string to = TopicAddress(topics[2].get<std::string>());
        std::string amount = HexToDec(log.value("data", "0"));
        std::unique_ptr<sql::PreparedStatement> c(conn.prepareStatement(
            "INSERT IGNORE INTO erc20_contracts(contract_address,deploy_tx_hash,deployer_address) VALUES(?,?,?)"));
        c->setString(1, contract); c->setString(2, tx.type == 7 ? tx.hash : "");
        c->setString(3, info.value("sender", "")); c->executeUpdate();
        std::unique_ptr<sql::PreparedStatement> event(conn.prepareStatement(
            "INSERT IGNORE INTO erc20_transfer_events(tx_hash,log_index,contract_address) VALUES(?,?,?)"));
        event->setString(1, tx.hash); event->setUInt(2, current_index); event->setString(3, contract);
        if (event->executeUpdate() == 0) continue;  // 区块重放/重启时不重复计算
        auto apply = [&](const std::string& account, bool add) {
            if (account.empty() || account == zero) return;
            std::unique_ptr<sql::PreparedStatement> q(conn.prepareStatement(
                "SELECT balance FROM erc20_balances WHERE contract_address=? AND account_address=? FOR UPDATE"));
            q->setString(1, contract); q->setString(2, account);
            std::unique_ptr<sql::ResultSet> rs(q->executeQuery());
            boost::multiprecision::cpp_int value = rs->next() ? boost::multiprecision::cpp_int(ToStd(rs->getString(1))) : 0;
            boost::multiprecision::cpp_int delta(amount); value += add ? delta : -delta;
            if (value < 0) throw std::runtime_error("ERC20 Transfer produced negative balance");
            std::unique_ptr<sql::PreparedStatement> u(conn.prepareStatement(
                "INSERT INTO erc20_balances(contract_address,account_address,balance) VALUES(?,?,?) ON DUPLICATE KEY UPDATE balance=VALUES(balance)"));
            u->setString(1, contract); u->setString(2, account); u->setString(3, value.convert_to<std::string>()); u->executeUpdate();
        };
        apply(from, false); apply(to, true);
    }
}

bool BalanceRepo::AssociateErc20(const std::string& account, const std::string& contract) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::unique_ptr<sql::PreparedStatement> exists(conn.prepareStatement("SELECT 1 FROM erc20_contracts WHERE contract_address=?"));
        exists->setString(1, Lower(contract)); std::unique_ptr<sql::ResultSet> rs(exists->executeQuery());
        if (!rs->next()) return false;
        std::unique_ptr<sql::PreparedStatement> p(conn.prepareStatement("INSERT IGNORE INTO account_erc20_contracts(account_address,contract_address) VALUES(?,?)"));
        p->setString(1, Lower(account)); p->setString(2, Lower(contract)); p->executeUpdate(); return true;
    });
}

std::vector<Erc20BalanceItem> BalanceRepo::ListErc20(const std::string& account) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::vector<Erc20BalanceItem> out;
        std::unique_ptr<sql::PreparedStatement> p(conn.prepareStatement(
            "SELECT a.contract_address,COALESCE(b.balance,'0') balance FROM account_erc20_contracts a LEFT JOIN erc20_balances b ON b.contract_address=a.contract_address AND b.account_address=a.account_address WHERE a.account_address=? ORDER BY a.created_at"));
        p->setString(1, Lower(account)); std::unique_ptr<sql::ResultSet> rs(p->executeQuery());
        while (rs->next()) out.push_back({ToStd(rs->getString(1)), ToStd(rs->getString(2))});
        return out;
    });
}

}  // namespace hubsql
