#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "model/transaction.h"

namespace sql {
class Connection;
}

namespace hubsql {

// 业务模块接口：一个业务 = 解析 + 存储 一体，注册进 BusinessRegistry 统一调度。
//
// 新增业务（注册制扩展）只需：
//   1. 定义记录结构（model/business/*.h）
//   2. 定义解析逻辑（parser/parsers/*）
//   3. 定义存储（storage/*_repo.*）
//   4. 实现本接口封装为模块（含 Process/List/Counts）
//   5. 在 main.cpp 中 Register 一次 —— 主流水线与 REST API 自动接入，无需改框架。
class IBusinessModule {
public:
    virtual ~IBusinessModule() = default;

    // 业务名（用于 API 路由查找，如 "staking" / "investment"）
    virtual std::string Name() const = 0;

    // 该模块是否处理此交易（快速跳过；按交易类型判断）
    virtual bool Handles(const Transaction& tx) const = 0;

    // 解析并落库一笔交易（在调用方提供的事务连接上执行），返回产出记录数
    virtual int Process(sql::Connection& conn, const Transaction& tx,
                        uint64_t block_height) = 0;

    // ---- REST API ----
    // 列表查询；filter 为业务自定义过滤参数（如 {"address":"...", "is_unstaked":1}）
    virtual nlohmann::json List(const nlohmann::json& filter, int page, int size) = 0;

    // 统计（并入 /api/v1/stats/overview）
    virtual nlohmann::json Counts() = 0;
};

}  // namespace hubsql
