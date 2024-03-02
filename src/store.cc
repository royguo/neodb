#include "store.h"

namespace neodb {
Status Store::Put(const std::string& key, std::shared_ptr<IOBuf> value) {
  zone_manager_->Append(key, value);
  return Status::OK();
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
    std::shared_ptr<IOBuf> read_value;
    zone_manager_->ReadSingleItem(lba, &read_key, read_value);
    if (key.compare(read_key) != 0) {
      return Status::IOError("loaded item's key not match!");
    }
    return Status::OK();
  }
}
}  // namespace neodb