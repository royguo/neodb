#pragma once

#include "neodb/options.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace neodb {
#define LOG(level, ...) LOG_##level(__VA_ARGS__)
#define LOG_DEBUG(...) spdlog::log(spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...) spdlog::log(spdlog::level::info, __VA_ARGS__)
#define LOG_WARNING(...) spdlog::log(spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) spdlog::log(spdlog::level::err, __VA_ARGS__)

void InitLogger(const LoggerOptions& options);

class LoggerInitializer {
 public:
  LoggerInitializer() { InitLogger(LoggerOptions()); }
};

static LoggerInitializer logger_initializer;
}  // namespace neodb
