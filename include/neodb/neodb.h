#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "store.h"

#define NEODB_VERSION "1.0.0"

namespace neodb {

class NeoDB {
 public:
  explicit NeoDB(DBOptions options) : options_(std::move(options)) {
    for (auto& store_options : options_.store_options_list_) {
      LOG(INFO, "Init store, device : {}", store_options.device_path_);
      stores_.emplace_back(new Store(store_options));
    }
  }

  ~NeoDB() = default;

 private:
  DBOptions options_;
  std::vector<std::unique_ptr<Store>> stores_;
};
}  // namespace neodb
