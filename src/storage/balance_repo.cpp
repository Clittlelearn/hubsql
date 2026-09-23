#include "storage/balance_repo.h"
#include "storage/erc20_parameter_parser.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <cctype>
#include <optional>

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
    const std::unordered_map<BalanceKey, boost::multiprecision::cpp_int,
                             BalanceKeyHash>& deltas) {
    if (deltas.empty()) return;

    pool_.WithConnection([&](sql::Connection& conn) {
        conn.setAutoCommit(false);
        try {
            for (const auto& [key, diff] : deltas) {
                const bool ohi = key.asset_type == "OHI";
                std::unique_ptr<sql::PreparedStatement> select(conn.prepareStatement(
                    ohi ? "SELECT balance FROM ohi_balances WHERE address=? FOR UPDATE"
                        : "SELECT balance FROM proposal_asset_balances WHERE address=? AND asset_type=? FOR UPDATE"));
                select->setString(1, key.address);
                if (!ohi) select->setString(2, key.asset_type);
                std::unique_ptr<sql::ResultSet> rs(select->executeQuery());
                const bool exists = rs->next();
                const boost::multiprecision::cpp_int current =
                    exists ? boost::multiprecision::cpp_int(ToStd(rs->getString(1))) : 0;
                const boost::multiprecision::cpp_int next = current + diff;
                if (next < 0) throw std::runtime_error("balance cannot be negative");

                if (exists) {
                    std::unique_ptr<sql::PreparedStatement> update(conn.prepareStatement(
                        ohi ? "UPDATE ohi_balances SET balance=? WHERE address=?"
                            : "UPDATE proposal_asset_balances SET balance=? WHERE address=? AND asset_type=?"));
                    update->setString(1, next.convert_to<std::string>());
                    update->setString(2, key.address);
                    if (!ohi) update->setString(3, key.asset_type);
                    update->executeUpdate();
                } else {
                    std::unique_ptr<sql::PreparedStatement> insert(conn.prepareStatement(
                        ohi ? "INSERT INTO ohi_balances(address,balance) VALUES(?,?)"
                            : "INSERT INTO proposal_asset_balances(address,asset_type,balance) VALUES(?,?,?)"));
                    insert->setString(1, key.address);
                    if (ohi) insert->setString(2, next.convert_to<std::string>());
                    else {
                        insert->setString(2, key.asset_type);
                        insert->setString(3, next.convert_to<std::string>());
                    }
                    insert->executeUpdate();
                }
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

std::optional<std::string> BalanceRepo::GetBalance(const std::string& addr,
                                                   const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::optional<std::string> {
        if (!asset_type.empty()) {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                asset_type == "OHI"
                    ? "SELECT balance FROM ohi_balances WHERE address = ?"
                    : "SELECT balance FROM proposal_asset_balances WHERE address = ? AND asset_type = ?"));
            pstmt->setString(1, addr);
            if (asset_type != "OHI") pstmt->setString(2, asset_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) return ToStd(res->getString(1));
            return std::nullopt;
        }
        // 未指定资产：返回该地址任意一条
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT balance FROM ohi_balances WHERE address = ? LIMIT 1"));
        pstmt->setString(1, addr);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) return ToStd(res->getString(1));
        return std::nullopt;
    });
}

std::vector<BalanceItem> BalanceRepo::ListAll(const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::vector<BalanceItem> {
        std::vector<BalanceItem> out;
        if (!asset_type.empty()) {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
                asset_type == "OHI"
                  ? "SELECT address, 'OHI' asset_type, balance FROM ohi_balances"
                  : "SELECT address, asset_type, balance FROM proposal_asset_balances WHERE asset_type=?"));
            if (asset_type != "OHI") pstmt->setString(1, asset_type);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                out.push_back({ToStd(res->getString("address")),
                               ToStd(res->getString("asset_type")),
                               ToStd(res->getString("balance"))});
            }
        } else {
            std::unique_ptr<sql::Statement> stmt(conn.createStatement());
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
                "SELECT address, asset_type, balance FROM (SELECT address,'OHI' asset_type,balance FROM ohi_balances UNION ALL SELECT address,asset_type,balance FROM proposal_asset_balances) b"));
            while (res->next()) {
                out.push_back({ToStd(res->getString("address")),
                               ToStd(res->getString("asset_type")),
                               ToStd(res->getString("balance"))});
            }
        }
        std::sort(out.begin(), out.end(), [](const BalanceItem& a, const BalanceItem& b) {
            return boost::multiprecision::cpp_int(a.balance) >
                   boost::multiprecision::cpp_int(b.balance);
        });
        return out;
    });
}

std::vector<BalanceItem> BalanceRepo::ListForAddress(const std::string& address) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::vector<BalanceItem> out;
        std::unique_ptr<sql::PreparedStatement> stmt(conn.prepareStatement(
            "SELECT address,'OHI' asset_type,balance FROM ohi_balances WHERE LOWER(address)=? "
            "UNION ALL SELECT address,asset_type,balance FROM proposal_asset_balances WHERE LOWER(address)=?"));
        stmt->setString(1, Lower(address));
        stmt->setString(2, Lower(address));
        std::unique_ptr<sql::ResultSet> rows(stmt->executeQuery());
        while (rows->next()) out.push_back({ToStd(rows->getString(1)), ToStd(rows->getString(2)), ToStd(rows->getString(3))});
        return out;
    });
}

std::string BalanceRepo::TotalBalance(const std::string& asset_type) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::string {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            asset_type.empty()
              ? "SELECT balance FROM (SELECT balance FROM ohi_balances UNION ALL SELECT balance FROM proposal_asset_balances) b"
              : asset_type == "OHI" ? "SELECT balance FROM ohi_balances"
              : "SELECT balance FROM proposal_asset_balances WHERE asset_type=?"));
        if (!asset_type.empty() && asset_type != "OHI") pstmt->setString(1, asset_type);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        boost::multiprecision::cpp_int total = 0;
        while (res->next()) total += boost::multiprecision::cpp_int(ToStd(res->getString(1)));
        return total.convert_to<std::string>();
    });
}

void BalanceRepo::ApplyErc20Transfers(sql::Connection& conn, const Transaction& tx,
                                      const nlohmann::json& execution_result) {
    if (tx.type != 7 && tx.type != 8) return;
    nlohmann::json info;
    try { info = nlohmann::json::parse(tx.data).value("txInfo", nlohmann::json::object()); }
    catch (...) { return; }
    const std::string zero = "0x0000000000000000000000000000000000000000";
    auto apply_transfer = [&](const std::string& contract,
                              const std::string& from,
                              const std::string& to,
                              const std::string& amount,
                              uint32_t event_index) {
        std::unique_ptr<sql::PreparedStatement> c(conn.prepareStatement(
            "INSERT IGNORE INTO erc20_contracts(contract_address,deploy_tx_hash,deployer_address) VALUES(?,?,?)"));
        c->setString(1, contract); c->setString(2, tx.type == 7 ? tx.hash : "");
        c->setString(3, info.value("sender", "")); c->executeUpdate();
        std::unique_ptr<sql::PreparedStatement> event(conn.prepareStatement(
            "INSERT IGNORE INTO erc20_transfer_events(tx_hash,log_index,contract_address) VALUES(?,?,?)"));
        event->setString(1, tx.hash); event->setUInt(2, event_index); event->setString(3, contract);
        if (event->executeUpdate() == 0) return;  // 区块重放/重启时不重复计算
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
    };

    // 部署初始发行、transfer/transferFrom、FlowIn/FlowOut 均以交易参数为
    // 金额与账户的主来源，即使节点没有 EVM log 也能正确入账。
    if (auto transfer = ParseErc20ParameterTransfer(tx, info)) {
        apply_transfer(transfer->contract, transfer->from, transfer->to,
                       transfer->amount, 0);
        return;
    }

    // 非直接调用（例如合约内部触发的 mint/burn/transfer）无法从外层参数
    // 确定资产变化，保留 EVM Transfer log 作为兼容回退。
    if (!execution_result.is_object()) return;
    const auto logs = execution_result.value("log", nlohmann::json::array());
    static const std::string transfer_topic = "ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef";
    uint32_t log_index = 0;
    for (const auto& log : logs) {
        const uint32_t current_index = log_index++;
        auto topics = log.value("topics", nlohmann::json::array());
        if (topics.size() < 3 || Lower(topics[0].get<std::string>()) != transfer_topic) continue;
        const std::string contract = TopicAddress(log.value("creator", ""));
        const std::string from = TopicAddress(topics[1].get<std::string>());
        const std::string to = TopicAddress(topics[2].get<std::string>());
        const std::string amount = HexToDec(log.value("data", "0"));
        apply_transfer(contract, from, to, amount, current_index);
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

bool BalanceRepo::RemoveErc20(const std::string& account, const std::string& contract) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::unique_ptr<sql::PreparedStatement> p(conn.prepareStatement(
            "DELETE FROM account_erc20_contracts WHERE account_address=? AND contract_address=?"));
        p->setString(1, Lower(account));
        p->setString(2, Lower(contract));
        return p->executeUpdate() > 0;
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

std::vector<Erc20BalanceItem> BalanceRepo::ListIndexedErc20(
    const std::string& account) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::vector<Erc20BalanceItem> out;
        std::unique_ptr<sql::PreparedStatement> p(conn.prepareStatement(
            "SELECT contract_address,balance FROM erc20_balances "
            "WHERE account_address=? ORDER BY contract_address"));
        p->setString(1, Lower(account));
        std::unique_ptr<sql::ResultSet> rs(p->executeQuery());
        while (rs->next()) {
            out.push_back({ToStd(rs->getString(1)), ToStd(rs->getString(2))});
        }
        return out;
    });
}

std::vector<AssetCatalogItem> BalanceRepo::ListAssetCatalog(const std::string& account) {
    return pool_.WithConnection([&](sql::Connection& conn) {
        std::vector<AssetCatalogItem> out;
        std::unique_ptr<sql::PreparedStatement> proposals(conn.prepareStatement(
            "SELECT asset,COALESCE(JSON_UNQUOTE(JSON_EXTRACT(tx_info,'$.name')),''),"
            "COALESCE(JSON_UNQUOTE(JSON_EXTRACT(tx_info,'$.tokenContractAddr')),''),"
            "COALESCE(CAST(JSON_UNQUOTE(JSON_EXTRACT(tx_info,'$.tokenDecimals')) AS UNSIGNED),8),"
            "EXISTS(SELECT 1 FROM contract_records cr WHERE cr.is_flow_in=1 AND cr.asset_type=proposals.asset),"
            "EXISTS(SELECT 1 FROM proposal_asset_balances b WHERE b.address=? AND b.asset_type=proposals.asset),"
            "native_flow_state "
            "FROM proposals ORDER BY is_first DESC,id"));
        proposals->setString(1, account);
        std::unique_ptr<sql::ResultSet> pr(proposals->executeQuery());
        while (pr->next()) {
            AssetCatalogItem item;
            item.kind = "proposal";
            item.asset_type = ToStd(pr->getString(1));
            item.asset_id = item.asset_type;
            item.name = ToStd(pr->getString(2));
            item.symbol = item.asset_type == "OHI" ? "OHI" : item.name;
            item.contract_address = Lower(ToStd(pr->getString(3)));
            item.decimals = pr->getInt(4);
            item.is_flow_in = pr->getBoolean(5);
            item.is_added = pr->getBoolean(6) || item.asset_type == "OHI";
            item.native_flow_state = ToStd(pr->getString(7));
            item.is_native_flow_active = item.native_flow_state == "active";
            out.push_back(std::move(item));
        }
        std::unique_ptr<sql::PreparedStatement> contracts(conn.prepareStatement(
            "SELECT c.contract_address,EXISTS(SELECT 1 FROM account_erc20_contracts a "
            "WHERE a.account_address=? AND a.contract_address=c.contract_address) "
            "FROM erc20_contracts c ORDER BY c.created_at,c.contract_address"));
        contracts->setString(1, Lower(account));
        std::unique_ptr<sql::ResultSet> cr(contracts->executeQuery());
        while (cr->next()) {
            AssetCatalogItem item;
            item.kind = "erc20";
            item.contract_address = Lower(ToStd(cr->getString(1)));
            item.asset_id = item.contract_address;
            item.name = "ERC20 " + item.contract_address.substr(0, 8);
            item.symbol = "TOKEN";
            item.decimals = 18;
            item.is_added = cr->getBoolean(2);
            out.push_back(std::move(item));
        }
        return out;
    });
}

}  // namespace hubsql
