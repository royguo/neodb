#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "store.h"
#include "trace_points.h"

#define NEODB_VERSION "1.0.0"

namespace neodb {

class NeoDB {
 public:
  explicit NeoDB(DBOptions options)
      : options_(std::move(options)), store_num_(options_.store_options_list_.size()) {
    for (int i = 0; i < options_.store_options_list_.size(); ++i) {
      auto store_options = options_.store_options_list_[i];
      LOG(INFO, "Init store {}, device name: {}", i, store_options.device_path_);
      stores_.emplace_back(new Store(store_options));
    }
  }

  ~NeoDB() = default;

  // memcpy Put
  Status Put(const std::string& key, const std::string& value);

  // zero-copy Put
  Status Put(const std::string& key, const std::shared_ptr<IOBuf>& value);

  // memcpy Get
  Status Get(const std::string& key, std::string* value);

  // zero-copy Get
  Status Get(const std::string& key, std::shared_ptr<IOBuf>& value);

  // Flush all existing buffer to the disk.
  //  Status Flush();

  std::shared_ptr<Index> DEBUG_GetIndex(int idx) { return stores_[idx]->DEBUG_GetIndex(); }

 private:
  DBOptions options_;

  // total number of device stores
  uint32_t store_num_ = 0;

  std::vector<std::unique_ptr<Store>> stores_;
};
}  // namespace neodb
