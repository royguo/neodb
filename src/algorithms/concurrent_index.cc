#include "concurrent_index.h"

#include "neodb/histogram.h"
#include "utils.h"

namespace neodb {

Status ConcurrentArt::Put(const std::string& key, uint64_t value) {
  uint32_t idx = GetHashedIndex(key);
  std::unique_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  return art_trees_[idx].Put(key, value);
}

Status ConcurrentArt::Get(const std::string& key, uint64_t* value) {
  uint32_t idx = GetHashedIndex(key);
  std::shared_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  return art_trees_[idx].Get(key, value);
}

bool ConcurrentArt::Exist(const std::string& key) {
  uint32_t idx = GetHashedIndex(key);
  std::shared_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  return art_trees_[idx].Exist(key);
}

Status ConcurrentArt::Delete(const std::string& key) {
  uint32_t idx = GetHashedIndex(key);
  std::unique_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  return art_trees_[idx].Delete(key);
}

// Concurrent hash map implementations.

Status ConcurrentHashMap::Put(const std::string& key, const std::shared_ptr<IOBuf>& value) {
  uint32_t idx = GetHashedIndex(key);
  std::unique_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  hash_maps_[idx][key] = value;
  return Status::OK();
}

Status ConcurrentHashMap::Get(const std::string& key, std::shared_ptr<IOBuf>& value) {
  uint32_t idx = GetHashedIndex(key);
  std::shared_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  auto hashmap = hash_maps_[idx];
  auto it = hashmap.find(key);
  if (it == hashmap.end()) {
    return Status::NotFound();
  }
  value = it->second;
  return Status::OK();
}

Status ConcurrentHashMap::Delete(const std::string& key) {
  uint32_t idx = GetHashedIndex(key);
  std::unique_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  auto hashmap = hash_maps_[idx];
  if (hashmap.erase(key) > 0) {
    return Status::OK();
  }
  return Status::NotFound();
}

bool ConcurrentHashMap::Exist(const std::string& key) {
  uint32_t idx = GetHashedIndex(key);
  std::shared_lock<std::shared_mutex> lk(mutex_arr_[idx]);
  auto hashmap = hash_maps_[idx];
  auto it = hashmap.find(key);
  if (it != hashmap.end()) {
    return true;
  }
  return false;
}

}  // namespace neodb