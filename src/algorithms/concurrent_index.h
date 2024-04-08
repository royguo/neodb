#pragma once
#include <unistd.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "art/art_wrapper.h"
#include "neodb/io_buf.h"
#include "neodb/status.h"
#include "utils.h"
namespace neodb {

// A concurrent & compacted hash map that optimized for fixed-size values.
class ConcurrentArt {
 public:
  explicit ConcurrentArt(uint32_t bucket_num) : bucket_num_(bucket_num) {
    mutex_arr_ = std::vector<std::shared_mutex>(bucket_num_);
    art_trees_.resize(bucket_num_);
  }
  ~ConcurrentArt() = default;

  Status Put(const std::string& key, uint64_t value);

  Status Get(const std::string& key, uint64_t* value);

  Status Delete(const std::string& key);

  bool Exist(const std::string& key);

 private:
  [[nodiscard]] inline uint32_t GetHashedIndex(const std::string& key) const {
    return HashUtils::FastHash(key) % bucket_num_;
  }

 private:
  uint32_t bucket_num_ = 10;
  std::vector<std::shared_mutex> mutex_arr_;
  std::vector<ARTWrapper> art_trees_;
};

// A simple concurrent hash map based on std::map.
class ConcurrentHashMap {
 public:
  explicit ConcurrentHashMap(uint32_t bucket_num) : bucket_num_(bucket_num) {
    mutex_arr_ = std::vector<std::shared_mutex>(bucket_num_);
    hash_maps_.resize(bucket_num_);
  }
  ~ConcurrentHashMap() = default;

  Status Put(const std::string& key, const std::shared_ptr<IOBuf>& value);

  Status Get(const std::string& key, std::shared_ptr<IOBuf>& value);

  Status Delete(const std::string& key);

  bool Exist(const std::string& key);

 private:
  [[nodiscard]] inline uint32_t GetHashedIndex(const std::string& key) const {
    return HashUtils::FastHash(key) % bucket_num_;
  }

 private:
  uint32_t bucket_num_ = 10;
  std::vector<std::shared_mutex> mutex_arr_;
  std::vector<std::unordered_map<std::string, std::shared_ptr<IOBuf>>> hash_maps_;
};

}  // namespace neodb