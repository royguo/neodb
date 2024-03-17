#include "store.h"

namespace neodb {
Status Store::Put(const std::string& key, const std::shared_ptr<IOBuf>& value) {
  return zone_manager_->Append(key, value);
}

Status Store::Get(const std::string& key, std::shared_ptr<IOBuf>& value) {
  Index::ValueVariant value_variant;
  auto s = index_->Get(key, value_variant);
  if (!s.ok()) {
    return s;
  }

  if (std::holds_alternative<Index::MemValue>(value_variant)) {
    value = (Index::MemValue)std::get<Index::MemValue>(value_variant);
    return Status::OK();
  } else {
    uint64_t lba = std::get<Index::LBAValue>(value_variant);
    std::string read_key;
    zone_manager_->ReadSingleItem(lba, &read_key, value);
    if (key != read_key) {
      return Status::IOError("Read single item error, key : " + key);
    }
    return Status::OK();
  }
}
}  // namespace neodb