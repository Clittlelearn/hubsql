#pragma once

#include <memory>
#include <string>
#include <vector>

#include "parser/ibusiness_module.h"

namespace sql {
class Connection;
}

namespace hubsql {

// 业务注册表：解析 + 存储 均通过注册的模块统一调度。
// 新增业务只需 Register 一个模块，流水线与 API 自动接入。
class BusinessRegistry {
public:
    void Register(std::shared_ptr<IBusinessModule> module);

    // 处理一笔交易：分发给所有 Handles(tx) 的模块，返回产出记录数（事务内调用）
    int ProcessTransaction(sql::Connection& conn, const Transaction& tx,
                           uint64_t block_height);

    // 按业务名查找模块；找不到返回 nullptr
    IBusinessModule* Find(const std::string& name) const;

    std::vector<IBusinessModule*> All() const;
    std::vector<std::string> Names() const;

private:
    std::vector<std::shared_ptr<IBusinessModule>> modules_;
};

}  // namespace hubsql
