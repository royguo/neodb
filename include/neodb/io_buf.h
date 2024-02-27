#pragma once

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace neodb {

#define PAGE_SIZE 4096

// IOBuf will always hold the ownership of the underlying data buffer.
// In contrast, a Slice will only hold references.
class IOBuf {
 public:
  IOBuf() = default;

  // Allocate memory for future use.
  explicit IOBuf(uint32_t capacity) : capacity_(capacity) {
    int ret = posix_memalign((void**)&buf_, PAGE_SIZE, capacity);
    if (ret != 0) {
      // TODO
    }
  }

  // Allocate memory & copy target string
  explicit IOBuf(const std::string& data) : IOBuf(data.size()) {
    Append(data.data(), data.size());
  }

  // Take the ownership of the input buffer.
  IOBuf(char* buf, uint32_t size) : capacity_(size), size_(size), buf_(buf) {}

  ~IOBuf() { free(buf_); }

  void Append(const char* src, uint32_t size) {
    memcpy(buf_ + size_, src, size);
    size_ += size;
  }

  // Skip a certain amount of bytes.
  void AppendZeros(uint32_t sz) {
    memset(buf_ + size_, 0, sz);
    size_ += sz;
  }

  void Append(const std::string& data) {
    memcpy(buf_ + size_, data.c_str(), data.size());
    size_ += data.size();
  }

  char* Buffer() { return buf_; }

  uint32_t Size() const { return size_; }

  uint32_t AvailableSize() const { return capacity_ - size_; }

  void IncreaseSize(uint32_t sz) { size_ += sz; }

  uint32_t Capacity() { return capacity_; }

  std::string Data() { return {buf_, size_}; }

  // Reset the write pointer, re-use the buffer.
  void Reset() { size_ = 0; }

 private:
  char* buf_ = nullptr;

  // current occupied size
  uint32_t size_ = 0;

  // Max usable size
  uint32_t capacity_ = 0;
};
}  // namespace neodb
