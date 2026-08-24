#include "parser/business_registry.h"

namespace hubsql {

void BusinessRegistry::Register(std::shared_ptr<IBusinessModule> module) {
    modules_.push_back(std::move(module));
}

int BusinessRegistry::ProcessTransaction(sql::Connection& conn,
                                         const Transaction& tx,
                                         uint64_t block_height) {
    int n = 0;
    for (const auto& m : modules_) {
        if (m->Handles(tx)) {
            n += m->Process(conn, tx, block_height);
        }
    }
    return n;
}

IBusinessModule* BusinessRegistry::Find(const std::string& name) const {
    for (const auto& m : modules_) {
        if (m->Name() == name) return m.get();
    }
    return nullptr;
}

std::vector<IBusinessModule*> BusinessRegistry::All() const {
    std::vector<IBusinessModule*> out;
    out.reserve(modules_.size());
    for (const auto& m : modules_) out.push_back(m.get());
    return out;
}

std::vector<std::string> BusinessRegistry::Names() const {
    std::vector<std::string> out;
    out.reserve(modules_.size());
    for (const auto& m : modules_) out.push_back(m->Name());
    return out;
}

}  // namespace hubsql
