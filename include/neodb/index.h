#pragma once

#include "neodb/io_buf.h"
#include "neodb/status.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

namespace neodb {

// The index may point to a data pointer, which address could possible be a
// memory address or SSD LBA address.
// The SSD LBA address is represented as follow:
//    64  =    45     +      12         +     7
//            ssd lba     size hint        reserved
//
// This is how the size hint works:
//  2  bits: page size of the hint (4KB/8KB/16KB/32KB)
//  10 bits: page number of the
//
// The maximum hint size is (32KB*2^10 = 32MB), if the actual value size is
// larger, we could use the size in record header.
//
// If the address is a memory address, then we should convert it into a char*
// pointer and read it from a data decoder.

// The actual data structure that handles the key value index
// The implementations should be thread-safe
class DataPointer {
 public:
  // TODO(royguo) We could use std::variant instead
  std::shared_ptr<IOBuf> mem_ptr;
  uint64_t disk_ptr;
  bool on_disk = false;
};

class IndexHandle {
 public:
  static const uint64_t kIndexNotFound;

 public:
  IndexHandle() = default;
  virtual ~IndexHandle() = default;

 public:
  virtual Status Put(const std::string& key, DataPointer ptr) = 0;

  virtual Status Get(const std::string& key, DataPointer* ptr) = 0;
};

class SimpleIndexHandle : public IndexHandle {
 public:
  SimpleIndexHandle() = default;

  ~SimpleIndexHandle() = default;

 public:
  Status Put(const std::string& key, DataPointer ptr) override {
    data_[key] = ptr;
    return Status::OK();
  }

  Status Get(const std::string& key, DataPointer* ptr) override {
    auto it = data_.find(key);
    if (it == data_.end()) {
      return Status::NotFound();
    }
    *ptr = data_[key];
    return Status::OK();
  }

 private:
  std::map<std::string, DataPointer> data_;
};

// Main index API entry
class Index {
 public:
  // The value is a pointer to the actual data, could be either in memory or
  // on SSD.
  Status Put(const std::string& key, DataPointer ptr);

  Status Get(const std::string& key, DataPointer* ptr);

 private:
  SimpleIndexHandle handle_;
};

}  // namespace neodb
