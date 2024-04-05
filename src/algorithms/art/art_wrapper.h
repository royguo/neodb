#include "art.h"

#include <cassert>

#include "neodb/status.h"

namespace neodb {
class ARTWrapper {
 public:
  ARTWrapper() { art_tree_init(&tree_); }
  ~ARTWrapper() { art_tree_destroy(&tree_); }

  Status Put(const std::string& key, uint64_t ptr) {
    auto* key_ptr = reinterpret_cast<const unsigned char*>(key.data());
    int key_sz = (int)key.size();
    // This is a special definition.
    // Since we are storing uint64_t as uintptr in the art index, but if the target value is 0, the
    // converted pointer would be NULL which is not we expected.
    // Since we don't want to change the foundation of ART index, so we use UINT64_MAX to represent
    // a user-side 0 value.
    // User will never use UINT64_MAX as actual data value because we will only use index for LBA.
    assert(ptr < UINT64_MAX);
    if (ptr == 0) {
      ptr = UINT64_MAX;
    }
    art_insert(&tree_, key_ptr, key_sz, (void*)ptr);
    return Status::OK();
  }

  Status Get(const std::string& key, uint64_t* ptr) {
    auto* key_ptr = reinterpret_cast<const unsigned char*>(key.data());
    int key_sz = (int)key.size();
    uint64_t ret_value_ptr = art_search(&tree_, key_ptr, key_sz);
    // We use UINT64_MAX to represent 0 value, refer to `Get()`.
    if (ret_value_ptr == UINT64_MAX) {
      *ptr = 0;
      return Status::OK();
    }
    if (nullptr != (void*)ret_value_ptr) {
      *ptr = uint64_t((void*)ret_value_ptr);
      return Status::OK();
    }
    return Status::NotFound();
  }

  Status Delete(const std::string& key) {
    auto* key_ptr = reinterpret_cast<const unsigned char*>(key.data());
    int key_sz = (int)key.size();
    void* ret = art_delete(&tree_, key_ptr, key_sz);
    if (ret == nullptr) {
      return Status::NotFound();
    }
    return Status::OK();
  }

  bool Exist(const std::string& key) {
    auto* key_ptr = reinterpret_cast<const unsigned char*>(key.data());
    int key_sz = (int)key.size();
    uint64_t ret_value_ptr = art_search(&tree_, key_ptr, key_sz);
    if ((void*)ret_value_ptr == nullptr) {
      return false;
    }
    return true;
  }

 private:
  art_tree tree_{};
};
}  // namespace neodb
