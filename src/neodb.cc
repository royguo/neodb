#include "neodb/neodb.h"

#include "utils.h"

namespace neodb {
Status NeoDB::Put(const std::string& key, const std::string& value) {
  auto value_ptr = std::make_shared<IOBuf>(value);
  return Put(key, value_ptr);
}

Status NeoDB::Put(const std::string& key, const std::shared_ptr<IOBuf>& value) {
  uint32_t store_idx = HashUtils::FastHash(key) % store_num_;
  return stores_[store_idx]->Put(key, value);
}

Status NeoDB::Get(const std::string& key, std::string* value) {
  std::shared_ptr<IOBuf> value_ptr;
  auto s = Get(key, value_ptr);
  if (!s.ok()) {
    return s;
  }
  *value = value_ptr->Data();
  return s;
}

Status NeoDB::Get(const std::string& key, std::shared_ptr<IOBuf>& value) {
  uint32_t store_idx = HashUtils::FastHash(key) % store_num_;
  return stores_[store_idx]->Get(key, value);
}

}  // namespace neodb