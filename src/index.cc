#include "neodb/index.h"

namespace neodb {

const uint64_t IndexHandle::kIndexNotFound = 0xffffffffffffffff;

Status Index::Put(const std::string& key, DataPointer ptr) {
  return handle_.Put(key, ptr);
}

Status Index::Get(const std::string& key, DataPointer* ptr) {
  return handle_.Get(key, ptr);
}

}  // neodb
