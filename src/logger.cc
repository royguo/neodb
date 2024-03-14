#include "logger.h"

#include <iostream>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace neodb {
void InitLogger(const LoggerOptions& options) {
  auto level = spdlog::level::from_str(options.level_);
  std::string path = options.log_path_.c_str();

  auto create_rotating_logger = [&options, level, path]() {
    // rotating logger
    auto logger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        fmt::format("{}/neodb.log", path.empty() ? "." : path),
        options.rotation_size_m_, options.max_backups_, true);
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

  spdlog::sinks_init_list sinks;
  if (!path.empty() && options.file_log_ && options.console_log_) {
    rotating_logger = create_rotating_logger();
    console_logger = create_console_logger();
    sinks = {rotating_logger, console_logger};
  } else if (!path.empty() && options.file_log_) {
    rotating_logger = create_rotating_logger();
    sinks = {rotating_logger};
  } else {
    console_logger = create_console_logger();
    sinks = {console_logger};
  }

  auto logger = std::make_shared<spdlog::logger>("neodb", sinks);
  logger->set_level(spdlog::level::debug);

  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%Y-%m-%H T%T.%e][%t][%l][%s:%#] %v");
  spdlog::info("logger initialized, log level: {}", options.level_);
}

}  // namespace neodb