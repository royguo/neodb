#include "logger.h"

#include <iostream>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace neodb {
LoggerInitializer::LoggerInitializer() {
  const LoggerOptions& options {};
  SPDLOG_INFO("logger initializing, log level: {}", options.level_);
  auto level = spdlog::level::from_str(options.level_);
  std::string path = options.log_path_;

  auto create_rotating_logger = [&options, level, path]() {
    // rotating logger
    auto logger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "neodb.log", options.rotation_size_m_, options.max_backups_, true);
    logger->set_level(level);
    return logger;
  };

  auto create_console_logger = [level]() {
    auto logger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger->set_level(level);
    return logger;
  };

  std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> rotating_logger;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_logger;

  std::vector<spdlog::sink_ptr> sinks;
  if (!path.empty() && options.file_log_) {
    sinks.push_back(create_rotating_logger());
  }
  if (options.console_log_) {
    sinks.push_back(create_console_logger());
  }

  auto combined_logger =
      std::make_shared<spdlog::logger>("neodb_logger", begin(sinks), end(sinks));
  spdlog::register_logger(combined_logger);
  spdlog::set_default_logger(combined_logger);
  combined_logger->set_level(level);
  combined_logger->set_pattern("[%Y-%m-%H T%T.%e][%t][%l][%s:%#] %v");

//  auto logger = std::make_share/*d<spdlog::logger>("neodb", sinks);
//  logger->set_level(spdlog::level::debug*/);

//  spdlog::set_default_logger(logger);
//  spdlog::set_pattern("[%Y-%m-%H T%T.%e][%t][%l][%s:%#] %v");
  SPDLOG_INFO("logger initialized, log level: {}", options.level_);
}

}  // namespace neodb
