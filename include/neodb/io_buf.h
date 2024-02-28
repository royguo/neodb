#pragma once

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "macros.h"

namespace neodb {

// IOBuf will always hold the ownership of the underlying data buffer.
// In contrast, a Slice will only hold references.
class IOBuf {
 public:
  IOBuf() = default;

  // Allocate memory for future use.
  explicit IOBuf(uint32_t capacity) : capacity_(capacity) {
    // capacity aligned to page size.
    capacity = (capacity + IO_PAGE_SIZE - 1) / IO_PAGE_SIZE * IO_PAGE_SIZE;
    int ret = posix_memalign((void**)&buf_, IO_PAGE_SIZE, capacity);
    if (ret != 0) {
      // TODO error handling?
    }
    original_buf_ = buf_;
  }

  // Allocate memory & copy target string
  explicit IOBuf(const std::string& data) : IOBuf(data.size()) {
    Append(data.data(), data.size());
  }

  // Take the ownership of the input buffer.
  IOBuf(char* buf, uint32_t size) : capacity_(size), size_(size), buf_(buf) {}

  ~IOBuf() { free(original_buf_); }

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

  // Shrink current buffer to [offset, offset + sz). After the shrinking
  // we cannot write to the buffer anymore.
  void Shrink(uint64_t offset, uint32_t sz) {
    assert(offset + sz <= capacity_);
    buf_ += offset;
    size_ = sz;
    capacity_ -= offset;
  }

  void AlignBufferSize() {
    uint64_t align = size_ % IO_PAGE_SIZE;
    if (align != 0) {
      size_ += (IO_PAGE_SIZE - align);
    }
  }

  char* Buffer() { return buf_; }

  uint32_t Size() const { return size_; }

  uint32_t AvailableSize() const { return capacity_ - size_; }

  void IncreaseSize(uint32_t sz) { size_ += sz; }

  uint32_t Capacity() const { return capacity_; }

  std::string Data() { return {buf_, size_}; }

  // Reset the write pointer, re-use the buffer.
  void Reset() { size_ = 0; }

 private:
  // The `buf_` could be changed if the IOBuf was shrunk.
  char* buf_ = nullptr;

  // The helper pointer to free the buffer, because `buf_` could point to
  // the middle of the original buffer after shrinking.
  char* original_buf_ = nullptr;

  // current occupied size
  uint32_t size_ = 0;

  // Max usable size.
  // If the buffer was shrunk, then the capacity will also be changed.
  uint32_t capacity_ = 0;
};
}  // namespace neodb
