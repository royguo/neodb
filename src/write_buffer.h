#pragma once

#include <unistd.h>

#include <mutex>

#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {
// A write buffer contains a set of KV items that was written to this system.
// If a write buffer exceeds its capacity, it will be convert into a immutable
// write buffer.
// WriteBuffer could be used in a multi-thread environment.
class WriteBuffer {
 public:
  using Item = std::pair<std::string, std::shared_ptr<IOBuf>>;

 public:
  WriteBuffer() = default;

  WriteBuffer(WriteBuffer&& buffer) {
    items_ = std::move(buffer.items_);
    capacity_bytes_ = buffer.capacity_bytes_;
    used_bytes_ = buffer.used_bytes_;
  }

  Status Put(const std::string& key, std::shared_ptr<IOBuf> value);

 private:
  std::vector<Item> items_;

  // We will calculate target encoded buffers size before the actual encoding.
  // Data layout is defined in `codec.h`.
  uint32_t expected_encoded_sz_ = 0;

  uint32_t capacity_bytes_ = 256UL << 20;

  uint32_t used_bytes_ = 0;

  bool immutable = false;

  std::mutex mtx_;
};
}  // namespace neodb