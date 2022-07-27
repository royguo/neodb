#pragma once

#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/slice.h"
#include "neodb/status.h"

#define NEODB_VERSION "1.0.0"

namespace neodb {

  class NeoDB {
    public:
    NeoDB(const DBOptions& opts): db_options_(opts) {}
    ~NeoDB() = default;

    DBOptions& Options() { return db_options_; }

    Status Put(const std::string& key, std::unique_ptr<IOBuf> value);

    // Get target value from DB, the underlying implementation will not take
    // over the ownership of the value.
    Status Get(const std::string& key, std::unique_ptr<IOBuf>* value) const;

    // Check if target key exist in database
    bool Peek(const std::string& key);

    Status Delete(const std::string& key);

    Status RecoverData();

    private:
    DBOptions db_options_;
  };
}