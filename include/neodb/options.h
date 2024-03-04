#pragma once
#include <unistd.h>

#include <filesystem>
#include <string>

namespace neodb {

// The options will be loaded on system start.
// StoreOptions represents a single device's options.
struct StoreOptions {
  std::string device_path_;

  uint64_t device_capacity_ = 10UL << 30;

  uint64_t device_zone_capacity_ = 20UL << 20;

  // Maximum capacity of each write buffer.
  // Total system memory usage:
  //  write_buffer_size_ * (writable_buffer_num_ * immutable_buffer_num_)
  uint32_t write_buffer_size_ = 256 << 20;

  // All writable buffers could be written in parallel.
  uint64_t writable_buffer_num_ = 10;

  uint64_t immutable_buffer_num_ = 10;
};

struct LoggerOptions {
  bool console_log_ = true;
  bool file_log_ = false;
  std::filesystem::path log_path_;
  std::string level_ = "debug";
  uint32_t rotation_size_m_ = 1000;
  uint32_t max_age_days_ = 7;
  uint32_t max_backups_ = 3;
};
}  // namespace neodb
