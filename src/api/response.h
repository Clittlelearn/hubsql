#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace hubsql {

// 统一响应结构: {code, message, data}
inline nlohmann::json Ok(nlohmann::json data = nlohmann::json()) {
    nlohmann::json j;
    j["code"] = 0;
    j["message"] = "ok";
    j["data"] = std::move(data);
    return j;
}

inline nlohmann::json Err(int code, const std::string& message) {
    nlohmann::json j;
    j["code"] = code;
    j["message"] = message;
    j["data"] = nullptr;
    return j;
}

}  // namespace hubsql
