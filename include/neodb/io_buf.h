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
    int ret = posix_memalign((void**)&buf_, PAGE_SIZE, capacity);
    if (ret != 0) {
      // TODO
    }
  }

  // Allocate memory & copy target string, useful for short string
  IOBuf(const std::string& data) : IOBuf(data.size()) {
    Append(data.data(), data.size());
  }

  ~IOBuf() { free(buf_); }

  void Append(const char* src, uint32_t size) {
    memcpy(buf_ + size_, src, size);
    size_ += size;
  }

  char* Buffer() { return buf_; }

  uint32_t Size() { return size_; }

  uint32_t Capacity() { return capacity_; }

  bool Sealed() { return capacity_ == size_; }

  std::string Data() { return std::string(buf_, size_);}

 private:
  // If buf is not nullptr, means we need to free space.
  char* buf_ = nullptr;

  // current occupied size
  uint32_t size_ = 0;

  // Max usable size
  uint32_t capacity_ = 0;
};
}  // namespace neodb
