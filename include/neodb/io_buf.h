#pragma once

#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace neodb {

#define PAGE_SIZE 4096

// IOBuf will always holds the ownership if the underlying data buffer.
// In contrast, a Slice will only holds references.
class IOBuf {
 public:
  IOBuf() {}

  IOBuf(uint32_t capacity) : capacity_(capacity) {
    assert(capacity % PAGE_SIZE == 0);
    int ret = posix_memalign((void**)&buf_, PAGE_SIZE, capacity);
    if (ret != 0) {
      // TODO
    }
  }

  ~IOBuf() { free(buf_); }

  void Append(char* src, uint32_t size) {
    memcpy(buf_ + size_, src, size);
    size_ += size;
  }

  char* Buffer() { return buf_; }

  uint32_t Size() { return size_; }

  uint32_t Capacity() { return capacity_; }

  bool Sealed() { return capacity_ == size_; }

  std::string Data() {}

 private:
  // If buf is not nullptr, means we need to free space.
  char* buf_ = nullptr;

  // current occupied size
  uint32_t size_ = 0;

  // Max usable size
  uint32_t capacity_ = 0;
};
}  // namespace neodb
