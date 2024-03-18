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
    lba_index_.emplace(key, std::get<LBAValue>(value));
  } else {
    mem_index_.emplace(key, std::get<MemValue>(value));
  }
}

Status Index::Get(const std::string& key, Index::ValueVariant& value) {
  tbb::concurrent_hash_map<std::string, std::shared_ptr<IOBuf>>::const_accessor mem_accessor;
  if (mem_index_.find(mem_accessor, key)) {
    value = mem_accessor->second;
    return Status::OK();
  }

  tbb::concurrent_hash_map<std::string, uint64_t>::const_accessor lba_accessor;
  if (lba_index_.find(lba_accessor, key)) {
    value = lba_accessor->second;
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
  return lba_index_.erase(key) || mem_index_.erase(key);
}

bool Index::Exist(const std::string& key) { return lba_index_.count(key) || mem_index_.count(key); }

bool Index::ExistInLBA(const std::string& key) { return lba_index_.count(key); }

}  // namespace neodb
