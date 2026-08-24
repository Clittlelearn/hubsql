#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 安全读取 JSON 字段：不存在/类型不符/为 null 时返回默认值
template <typename T>
T GetOr(const nlohmann::json& j, const std::string& key, T def) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) {
        try {
            return it->get<T>();
        } catch (...) {
            // fallthrough
        }
    }
    return def;
}

inline std::string GetStr(const nlohmann::json& j, const std::string& key,
                          std::string def = "") {
    return GetOr<std::string>(j, key, std::move(def));
}

inline uint64_t GetU64(const nlohmann::json& j, const std::string& key,
                       uint64_t def = 0) {
    return GetOr<uint64_t>(j, key, def);
}

inline double GetDouble(const nlohmann::json& j, const std::string& key,
                        double def = 0.0) {
    return GetOr<double>(j, key, def);
}

}  // namespace hubsql
