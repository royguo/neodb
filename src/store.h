#pragma once

#include <string>
#include <vector>

#include "disk.h"
#include "neodb/status.h"

namespace neodb {
class StoreOptions {};

class Store {
 public:
  Store(StoreOptions options);

  Status Put(const std::string& key);

  Status Get();

  bool Exist();

 private:
  std::vector<Disk> disks_;
};
}  // namespace neodb