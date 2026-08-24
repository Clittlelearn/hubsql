#include "common/logger.h"

#include <filesystem>
#include <memory>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace hubsql {

namespace {

spdlog::level::level_enum ParseLevel(const std::string& level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "warn")  return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    return spdlog::level::info;
}

}  // namespace

void Logger::Init(const LogConfig& cfg) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    if (!cfg.file.empty()) {
        // 确保日志目录存在
        std::filesystem::create_directories(
            std::filesystem::path(cfg.file).parent_path());
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            cfg.file, 10 * 1024 * 1024, 5));
    }

    auto logger = std::make_shared<spdlog::logger>("hubsql", sinks.begin(), sinks.end());
    logger->set_level(ParseLevel(cfg.level));
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
}

}  // namespace hubsql
