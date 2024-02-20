#pragma once
#include <string>
#include <unistd.h>

namespace neodb {

// The options will be loaded on system start.
struct DBOptions {
  bool use_existing_db = true;
  // DB path could be either a absolute file path or a block devices path.
  std::string db_path;

  // Maximum capacity of each write buffer.
  // Total system memory usage:
  //  write_buffer_size * (writable_buffer_num * immutable_buffer_num)
  uint32_t write_buffer_size = 256 << 20;

  // All writable buffers could be written in parallel.
  uint64_t writable_buffer_num = 10;

  uint64_t immutable_buffer_num = 10;
};
}
