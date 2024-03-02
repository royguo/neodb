#pragma once

#include <string>
#include <vector>

#include "index.h"
#include "io.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "zone_manager.h"

namespace neodb {

// Single device storage engine.
class Store {
 public:
  explicit Store(StoreOptions options) : options_(options) {
    auto io_handle = std::make_unique<FileIOHandle>(
        options.device_path_, options.device_capacity_,
        options.device_zone_capacity_);
    zone_manager_ =
        std::make_unique<ZoneManager>(options, std::move(io_handle), index_);
  }

  ~Store() = default;

  // Put an already exist key will simply overwrite the old one.
  Status Put(const std::string& key, std::shared_ptr<IOBuf> value);

  Status Get(const std::string& key, std::shared_ptr<IOBuf>& value);

 private:
  StoreOptions options_;
  std::shared_ptr<Index> index_;
  std::unique_ptr<ZoneManager> zone_manager_;
};
}  // namespace neodb