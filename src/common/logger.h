#pragma once

#include <string>
#include <spdlog/spdlog.h>

#include "common/config.h"

namespace hubsql {

// 日志初始化：控制台 + 滚动文件，级别从配置读取
class Logger {
public:
    static void Init(const LogConfig& cfg);
};

}  // namespace hubsql

#define LOG_TRACE(...) ::spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)  ::spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)  ::spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) ::spdlog::error(__VA_ARGS__)
