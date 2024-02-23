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

  WriteBuffer(WriteBuffer&& buffer) noexcept {
    items_ = std::move(buffer.items_);
    capacity_bytes_ = buffer.capacity_bytes_;
    used_bytes_ = buffer.used_bytes_;
  }

  Status Put(const std::string& key, const std::shared_ptr<IOBuf>& value);

  bool IsFull() const { return used_bytes_ >= capacity_bytes_; }

  bool IsImmutable() const { return immutable; }

  void MarkImmutable() { immutable = true; }

  std::vector<Item>* GetItems() { return &items_; }

 private:
  std::vector<Item> items_;

  uint32_t capacity_bytes_ = 256UL << 20;

  uint32_t used_bytes_ = 0;

  bool immutable = false;
};
}  // namespace neodb