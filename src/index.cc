#include "index.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {
void Index::Put(const std::string& key, const Index::ValueVariant& value) {
  std::unique_lock<std::mutex> lk(mtx_);
  if (std::holds_alternative<LBAValue>(value)) {
    lba_index_.emplace(key, std::get<LBAValue>(value));
  } else {
    mem_index_.emplace(key, std::get<MemValue>(value));
  }
}

Status Index::Get(const std::string& key, Index::ValueVariant& value) {
  std::unique_lock<std::mutex> lk(mtx_);
  auto mem_it = mem_index_.find(key);
  if (mem_it != mem_index_.end()) {
    value = mem_it->second;
    return Status::OK();
  }

  auto lba_it = lba_index_.find(key);
  if (lba_it != lba_index_.end()) {
    value = lba_it->second;
    return Status::OK();
  }
  return Status::NotFound("key not found");
}

void Index::Update(const std::string& key, const Index::ValueVariant& value) {
  Delete(key);
  Put(key, value);
}

void Index::Delete(const std::string& key) {
  std::unique_lock<std::mutex> lk(mtx_);
  auto lba_it = lba_index_.find(key);
  if (lba_it != lba_index_.end()) {
    lba_index_.erase(lba_it);
    return;
  }

  auto mem_it = mem_index_.find(key);
  if (mem_it != mem_index_.end()) {
    mem_index_.erase(mem_it);
  }
}

bool Index::Exist(const std::string& key) {
  std::unique_lock<std::mutex> lk(mtx_);
  return lba_index_.find(key) != lba_index_.end() ||
         mem_index_.find(key) != mem_index_.end();
}

}  // namespace neodb
