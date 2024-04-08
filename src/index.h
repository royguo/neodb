#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "algorithms/concurrent_index.h"
#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {

class Index {
 public:
  // Memory index value.
  using MemValue = std::shared_ptr<IOBuf>;
  // LBA index value, colored pointer.
  // [7 bits, target item's size indicator, maximum 512KB]
  // [1 bit, pinned or not]
  // [11 access frequency indicator]
  // [45 bits, LBA, maximum: 32TB]
  using LBAValue = uint64_t;
  // Index could possibly point to either memory or lba.
  using ValueVariant = std::variant<MemValue, LBAValue>;

 public:
  Index() {
    mem_index_ = std::make_unique<ConcurrentHashMap>(20);
    lba_index_ = std::make_unique<ConcurrentArt>(20);
  }
  ~Index() = default;

  void Put(const std::string& key, const ValueVariant& value);

  Status Get(const std::string& key, ValueVariant& value);

  bool Update(const std::string& key, const ValueVariant& value);

  bool Delete(const std::string& key);

  bool Exist(const std::string& key);

  bool ExistInLBA(const std::string& key);

 private:
  std::unique_ptr<ConcurrentHashMap> mem_index_;
  //  tbb::concurrent_hash_map<std::string, std::shared_ptr<IOBuf> > mem_index_;
  //  std::unordered_map<std::string, std::shared_ptr<IOBuf>> mem_index_;

  std::unique_ptr<ConcurrentArt> lba_index_;
  //  tbb::concurrent_hash_map<std::string, uint64_t> lba_index_;
  //  std::unordered_map<std::string, uint64_t> lba_index_;
  std::mutex mtx_;
};
}  // namespace neodb
