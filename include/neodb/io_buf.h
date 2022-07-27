#pragma once

#include <memory>
#include <unistd.h>

namespace neodb {
// IOBuf manage buffer memory itself.
class IOBuf {
 public:
  static std::unique_ptr<IOBuf> TakeOwnership(char* buf, uint32_t size) {
    return std::make_unique<IOBuf>(buf, size);
  }

 public:
  // Take the ownership of target buffer, the capacity is set to target size and
  // not changeable.
  IOBuf(char* buf, uint32_t size) : buf_(buf), size_(size), capacity_(size) {}

  // Reuse input buffer, take over the ownership (zero copy)
  IOBuf(std::string data) : data_(std::move(data)) {
    size_ = data.size();
    capacity_ = data.size();
  }

  // TODO allocate a new buffer for further use.
  IOBuf(uint32_t capacity) : capacity_(capacity) {}

  // Delete copy constructor, you could only use IOBuf via smart pointers.
  IOBuf(const IOBuf& buf) = delete;

  ~IOBuf() {
    if (IsManagedBuffer()) {
      free(buf_);
    }
  }

  char* Buffer() { return buf_; }

  uint32_t Size() { return size_; }

  uint32_t Capacity() { return capacity_; }

  bool Sealed() { return capacity_ == size_; }

  // Copy append new data to the end of the buffer.
  int Append(char* src, uint32_t length) { return length; }

  std::string Data() {
    if (IsManagedBuffer()) {
      return std::string(buf_, size_);
    }
    return data_;
  }

 private:
  // Do we need to free the buffer on destruction of current IOBuf.
  bool IsManagedBuffer() {
    return (buf_ != nullptr && data_.empty() && size_ > 0 && capacity_ > 0);
  }

 private:
  // If buf is not nullptr, means we need to free space.
  char* buf_ = nullptr;

  uint32_t size_ = 0;
  uint32_t capacity_ = 0;

  std::string data_;
};
}  // namespace neodb