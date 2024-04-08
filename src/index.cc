#include "index.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {
void Index::Put(const std::string& key, const Index::ValueVariant& value) {
  if (std::holds_alternative<LBAValue>(value)) {
    lba_index_->Put(key, std::get<LBAValue>(value));
  } else {
    mem_index_->Put(key, std::get<MemValue>(value));
  }
}

Status Index::Get(const std::string& key, Index::ValueVariant& value) {
  // If the target value presents in memory
  std::shared_ptr<IOBuf> ret_value_mem;
  auto s = mem_index_->Get(key, ret_value_mem);
  if (s.ok()) {
    value = ret_value_mem;
    return Status::OK();
  }

  // or in LBA
  uint64_t ret_value_lba = 0;
  s = lba_index_->Get(key, &ret_value_lba);
  if (s.ok()) {
    value = ret_value_lba;
    return Status::OK();
  }
  return Status::NotFound("key not found : " + key);
}

// @return True if update success, False if key not found.
bool Index::Update(const std::string& key, const Index::ValueVariant& value) {
  // Only if the key is still valid, we can update it.
  if (Delete(key)) {
    Put(key, value);
    return true;
  }
  return false;
}

bool Index::Delete(const std::string& key) {
  return lba_index_->Delete(key).ok() || mem_index_->Delete(key).ok();
}

bool Index::Exist(const std::string& key) {
  return lba_index_->Exist(key) || mem_index_->Exist(key);
}

bool Index::ExistInLBA(const std::string& key) { return lba_index_->Exist(key); }

}  // namespace neodb
