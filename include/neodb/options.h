#pragma once

namespace neodb {
struct DBOptions {
  bool use_existing_db = true;
  // DB path could be either a absolute file path or a block devices path.
  std::string db_path;
  // Total number of the buffers for user requests
  uint32_t user_buffers = 2;
  // Total number of gc buffers for data migration
  uint32_t gc_buffers = 1;
  // Size in byte of each buffer
  uint32_t max_buffer_size = 128 << 20;
};
}