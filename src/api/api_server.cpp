#include "api/api_server.h"

#include <cstdlib>
#include <cstdint>
#include <string>

#include <crow.h>
#include <crow/middlewares/cors.h>

#include "api/controllers/balance_controller.h"
#include "api/controllers/block_controller.h"
#include "api/controllers/business_controller.h"
#include "api/controllers/tx_controller.h"
#include "api/response.h"
#include "common/logger.h"

namespace hubsql {

namespace {

// 查询参数读取辅助
int GetInt(const crow::request& req, const char* key, int def) {
    const char* v = req.url_params.get(key);
    return v ? std::atoi(v) : def;
}

uint64_t GetU64(const crow::request& req, const char* key, uint64_t def) {
    const char* v = req.url_params.get(key);
    return v ? std::strtoull(v, nullptr, 10) : def;
}

std::string GetStr(const crow::request& req, const char* key,
                   const std::string& def = "") {
    const char* v = req.url_params.get(key);
    return v ? std::string(v) : def;
}

// 统一 JSON 响应
crow::response JsonRespond(const nlohmann::json& body) {
    crow::response r(200, body.dump());
    r.add_header("Content-Type", "application/json");
    r.add_header("Access-Control-Allow-Origin", "*");
    r.add_header("Access-Control-Allow-Headers", "Content-Type");
    r.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    return r;
}

crow::response CorsPreflight() {
    crow::response r(204);
    r.add_header("Access-Control-Allow-Origin", "*");
    r.add_header("Access-Control-Allow-Headers", "Content-Type");
    r.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    return r;
}

}  // namespace

struct ApiServer::Impl {
    ApiConfig cfg;
    crow::App<crow::CORSHandler> app;
};

ApiServer::ApiServer(const ApiConfig& cfg, DbPool& pool,
                     BlockRepo& blocks, TxRepo& txs, BusinessRegistry& registry,
                     BalanceRepo& balances, const std::string& rpc_url)
    : impl_(std::make_unique<Impl>()) {
    impl_->cfg = cfg;
    impl_->app.get_middleware<crow::CORSHandler>()
        .global()
        .origin("*")
        .headers("Content-Type")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST,
                 crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS);
    BlockController block_ctrl(blocks);
    TxController tx_ctrl(txs);
    BusinessController business_ctrl(registry, blocks, txs);
    BalanceController balance_ctrl(balances, rpc_url);

    // ---- 健康检查 ----
    impl_->app.route_dynamic("/health")([](const crow::request&) {
        return JsonRespond(Ok({{"status", "healthy"}}));
    });

    // ---- 区块 ----
    impl_->app.route_dynamic("/api/v1/blocks")
        .methods(crow::HTTPMethod::GET)(
            [block_ctrl](const crow::request& req) mutable {
                return JsonRespond(block_ctrl.List(
                    GetU64(req, "start", 0), GetU64(req, "end", UINT64_MAX),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/blocks/<uint>")
        .methods(crow::HTTPMethod::GET)(
            [block_ctrl](const crow::request&, uint64_t height) mutable {
                return JsonRespond(block_ctrl.Detail(height));
            });

    // ---- 交易 ----
    impl_->app.route_dynamic("/api/v1/txs")
        .methods(crow::HTTPMethod::GET)(
            [tx_ctrl](const crow::request& req) mutable {
                return JsonRespond(tx_ctrl.List(
                    GetStr(req, "type"), GetStr(req, "address"),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/txs/<string>")
        .methods(crow::HTTPMethod::GET)(
            [tx_ctrl](const crow::request&, std::string tx_hash) mutable {
                return JsonRespond(tx_ctrl.Detail(tx_hash));
            });

    // ---- 业务数据（注册制：按注册的业务模块名分发）----
    // 通用: /api/v1/business/{name}?address=&is_unstaked=&page=&size=
    // 别名: /api/v1/staking /api/v1/unstaking /api/v1/investments
    auto build_filter = [](const crow::request& req) {
        nlohmann::json f = nlohmann::json::object();
        if (req.url_params.get("address"))
            f["address"] = GetStr(req, "address");
        if (req.url_params.get("is_unstaked"))
            f["is_unstaked"] = GetInt(req, "is_unstaked", -1);
        if (req.url_params.get("is_deinvested"))
            f["is_deinvested"] = GetInt(req, "is_deinvested", -1);
        if (req.url_params.get("is_revoked"))
            f["is_revoked"] = GetInt(req, "is_revoked", -1);
        if (req.url_params.get("is_unlocked"))
            f["is_unlocked"] = GetInt(req, "is_unlocked", -1);
        if (req.url_params.get("proposal_hash"))
            f["proposal_hash"] = GetStr(req, "proposal_hash");
        if (req.url_params.get("type"))
            f["type"] = GetStr(req, "type");
        if (req.url_params.get("tx_type"))
            f["tx_type"] = GetStr(req, "tx_type");
        if (req.url_params.get("asset_type"))
            f["asset_type"] = GetStr(req, "asset_type");
        if (req.url_params.get("contract_address"))
            f["contract_address"] = GetStr(req, "contract_address");
        if (req.url_params.get("is_flow_in"))
            f["is_flow_in"] = GetInt(req, "is_flow_in", -1);
        if (req.url_params.get("is_flow_out"))
            f["is_flow_out"] = GetInt(req, "is_flow_out", -1);
        return f;
    };

    impl_->app.route_dynamic("/api/v1/business/<string>")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req,
                                          std::string name) mutable {
                return JsonRespond(business_ctrl.List(
                    name, build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/staking")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "staking", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/unstaking")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                nlohmann::json f = build_filter(req);
                f["is_unstaked"] = 1;  // 已解质押视图
                return JsonRespond(business_ctrl.List(
                    "staking", f,
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/investments")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "investment", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/deinvestments")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                nlohmann::json f = build_filter(req);
                f["is_deinvested"] = 1;  // 已解投资视图
                return JsonRespond(business_ctrl.List(
                    "investment", f,
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 提案 / 投票 ----
    impl_->app.route_dynamic("/api/v1/proposals")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "proposal", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/revokedproposals")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                nlohmann::json f = build_filter(req);
                f["is_revoked"] = 1;  // 已撤销提案视图
                return JsonRespond(business_ctrl.List(
                    "proposal", f,
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/votes")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "vote", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 锁定 / 解锁定 ----
    impl_->app.route_dynamic("/api/v1/locks")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "lock", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/unlockedlocks")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                nlohmann::json f = build_filter(req);
                f["is_unlocked"] = 1;  // 已解锁定视图
                return JsonRespond(business_ctrl.List(
                    "lock", f,
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 交易记录（滚动，仅最新 20 个高度）----
    impl_->app.route_dynamic("/api/v1/txrecords")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "txrecord", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 合约交易（含跃入/跃出）----
    impl_->app.route_dynamic("/api/v1/contracts")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "contract", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 申领 ----
    impl_->app.route_dynamic("/api/v1/claims")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "claim", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- ERC20 资金发放（transfer）----
    impl_->app.route_dynamic("/api/v1/funds")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl, build_filter](const crow::request& req) mutable {
                return JsonRespond(business_ctrl.List(
                    "fund", build_filter(req),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });

    // ---- 统计（聚合所有已注册业务模块）----
    impl_->app.route_dynamic("/api/v1/stats/overview")
        .methods(crow::HTTPMethod::GET)(
            [business_ctrl](const crow::request&) mutable {
                return JsonRespond(business_ctrl.Stats());
            });

    // ---- 账户余额 ----
    impl_->app.route_dynamic("/api/v1/balances")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request& req) mutable {
                return JsonRespond(balance_ctrl.List(
                    GetStr(req, "asset_type"),
                    GetInt(req, "page", 1), GetInt(req, "size", 20)));
            });
    impl_->app.route_dynamic("/api/v1/balances/<string>")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request& req, std::string address) mutable {
                return JsonRespond(balance_ctrl.Get(
                    address, GetStr(req, "asset_type")));
            });

    // 用户显式关联后，才在账户 ERC20 余额列表中展示该合约。
    impl_->app.route_dynamic("/api/v1/accounts/<string>/erc20-contracts")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)(
            [balance_ctrl](const crow::request& req, std::string address) mutable {
                if (req.method == crow::HTTPMethod::OPTIONS) return CorsPreflight();
                try {
                    auto body = nlohmann::json::parse(req.body);
                    return JsonRespond(balance_ctrl.AssociateErc20(
                        address, body.value("contract_address", "")));
                } catch (...) { return JsonRespond(Err(400, "invalid JSON body")); }
            });
    impl_->app.route_dynamic("/api/v1/accounts/<string>/erc20-balances")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request&, std::string address) mutable {
                return JsonRespond(balance_ctrl.ListErc20(address));
            });
    // Flow 等业务读取区块索引余额，不受 Wallet 的“添加 Token”关系限制。
    impl_->app.route_dynamic("/api/v1/accounts/<string>/indexed-erc20-balances")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request&, std::string address) mutable {
                return JsonRespond(balance_ctrl.ListIndexedErc20(address));
            });
    impl_->app.route_dynamic("/api/v1/accounts/<string>/erc20-contracts/<string>")
        .methods(crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS)(
            [balance_ctrl](const crow::request& req, std::string address,
                           std::string contract) mutable {
                if (req.method == crow::HTTPMethod::OPTIONS) return CorsPreflight();
                return JsonRespond(balance_ctrl.RemoveErc20(address, contract));
            });
    impl_->app.route_dynamic("/api/v1/assets/catalog")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request& req) mutable {
                return JsonRespond(balance_ctrl.AssetCatalog(GetStr(req, "address")));
            });
    impl_->app.route_dynamic("/api/v1/assets/<string>/metadata")
        .methods(crow::HTTPMethod::GET)(
            [balance_ctrl](const crow::request&, std::string contract) mutable {
                return JsonRespond(balance_ctrl.TokenMetadata(std::move(contract)));
            });

    impl_->app.port(cfg.port).bindaddr(cfg.host).multithreaded();
}

ApiServer::~ApiServer() = default;

void ApiServer::Run() {
    LOG_INFO("REST API 启动: {}:{}", impl_->cfg.host, impl_->cfg.port);
    impl_->app.run();
}

void ApiServer::Stop() {
    impl_->app.stop();
}

}  // namespace hubsql
