#pragma once

#include "neodb/options.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace neodb {
#define LOG(level, ...) LOG_##level(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARNING(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)

void InitLogger(const LoggerOptions& options);

class LoggerInitializer {
 public:
  LoggerInitializer() { InitLogger(LoggerOptions()); }
};

static LoggerInitializer logger_initializer;
}  // namespace neodb
