#pragma once

#include <memory>
#include <vector>

#include "../../src/deprecated/write_buffer_bak.h"
#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/slice.h"
#include "neodb/status.h"

#define NEODB_VERSION "1.0.0"

namespace neodb {

class NeoDB {
 public:
  NeoDB() {}

  NeoDB(const DBOptions& opts) : db_options_(opts) {
    auto callback = [&](const std::string& key, uint64_t disk_ptr) {};
    write_buffer_.reset(
        new WriteBuffer(opts.buffer_size, opts.queue_num, callback));
  }

  ~NeoDB() = default;

  DBOptions& Options() { return db_options_; }

  // We must enable zero-copy, since our target use case is for large values.
  Status Put(const std::string& key, std::shared_ptr<IOBuf> value);

  // Get target value from DB, the underlying implementation will not take
  // over the ownership of the value.
  Status Get(const std::string& key, std::shared_ptr<IOBuf>* value);

  // Check if target key exist in database
  bool Peek(const std::string& key);

  Status Delete(const std::string& key);

  Status RecoverData();

 private:
  DBOptions db_options_;

  std::shared_ptr<Index> index_;

  // holds all incoming write records
  std::shared_ptr<WriteBuffer> write_buffer_;
};
}
