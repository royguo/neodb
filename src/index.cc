#include "neodb/index.h"

namespace neodb {

const uint64_t Index::kIndexNotFound = 0xffffffffffffffff;

void Index::Put(const std::string& key, Index::ValueType value) {
  idx_[key] = std::move(value);
}

Index::ValueType Index::Get(const std::string& key) {
  auto it = idx_.find(key);
  if (it != idx_.end()) {
    return it->second;
  }
  return Index::kIndexNotFound;
}
}