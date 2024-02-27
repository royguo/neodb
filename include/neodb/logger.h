#pragma once

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace neodb {

// enum LogLevel { DEBUG, INFO, ERROR };
//
// class Logger {
//  public:
//   explicit Logger(LogLevel level) : level_(level) {
//     spdlog_->set_level(spdlog::level::trace);
//   };
//
//   static std::shared_ptr<spdlog::logger> GetLogger(LogLevel level) {
//     static Logger logger(level);
//     return logger.spdlog_;
//   }
//
//  private:
//   uint64_t max_size = 1UL << 30;
//   uint32_t max_files = 10;
//   LogLevel level_;
//   std::shared_ptr<spdlog::logger> spdlog_ = spdlog::rotating_logger_mt(
//       "some_logger_name", "logs/rotating.txt", max_size, max_files);
//   ;
// };

#define LOG(level, ...) LOG_##level(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)

}  // namespace neodb
