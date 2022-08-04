#include "neodb/index.h"

namespace neodb {

const uint64_t Index::kIndexNotFound = 0xffffffffffffffff;

Status Index::Put(const std::string& key, DataPointer pointer) {
  return handle_.Put(key, pointer);
}

Status Index::Get(const std::string& key, DataPointer* pointer) {
  return handle_.Get(key, pointer);
}

}  // neodb