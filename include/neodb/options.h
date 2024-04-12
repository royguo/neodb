#pragma once
#include <unistd.h>

#include <vector>
#include <string>

namespace neodb {

struct StoreOptions;

enum DeviceType { kFile, kBlock };

struct DBOptions {
  std::vector<StoreOptions> store_options_list_;
};

// The options will be loaded on system start.
// StoreOptions represents a single device's options.
struct StoreOptions {
  std::string name_ = "device1";

  std::string device_path_;

  // The device path could be a sub-range of the hardware or file.
  uint64_t device_offset_ = 0;

  DeviceType type_ = kFile;

  uint64_t device_capacity_ = 10UL << 30;

  uint64_t device_zone_capacity_ = 20UL << 20;

  // Maximum capacity of each write buffer.
  // Total system memory usage:
  //  write_buffer_size_ * (writable_buffer_num_ * immutable_buffer_num_)
  uint32_t write_buffer_size_ = 128 << 20;

  // All writable buffers could be written in parallel.
  uint64_t writable_buffer_num_ = 10;

  uint64_t immutable_buffer_num_ = 10;

  // If empty zone number less than this, we should trigger GC.
  uint32_t gc_threshold_zone_num_ = 3;

  bool recover_exist_db_ = false;
};

struct LoggerOptions {
  bool console_log_ = true;
  bool file_log_ = false;
  std::string log_path_;
#ifdef NDEBUG
  std::string level_ = "info";
#else
  std::string level_ = "debug";
#endif
  uint32_t rotation_size_m_ = 1000;
  uint32_t max_age_days_ = 7;
  uint32_t max_backups_ = 3;
};
}  // namespace neodb
