#pragma once

namespace neodb {
struct DBOptions {
  bool use_existing_db = true;
  // DB path could be either a absolute file path or a block devices path.
  std::string db_path;

  // total buffer size
  uint32_t buffer_size = 256 << 20;

  // how many underlying queue inside the write buffer(they will share the max
  // buffer size)
  int queue_num = 4;
};
}
