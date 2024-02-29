#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {

class Index {
 public:
  // Memory index value.
  using MemValue = std::shared_ptr<IOBuf>;
  // LBA index value, colored pointer.
  // [16 bits, target value size in pages, maximum: 256 MB]
  // [1 bit, pinned or not]
  // [2 reserved]
  // [45 bits, LBA, maximum: 32TB]
  using LBAValue = uint64_t;
  // Index could possibly point to either memory or lba.
  using ValueVariant = std::variant<MemValue, LBAValue>;

 public:
  Index() = default;
  ~Index() = default;

  void Put(const std::string& key, const ValueVariant& value);

  Status Get(const std::string& key, ValueVariant* value);

  void Update(const std::string& key, const ValueVariant& value);

  void Delete(const std::string& key);

  bool Exist(const std::string& key);

 private:
  // TODO(Roy Guo) Use better index data structures.
  std::unordered_map<std::string, std::shared_ptr<IOBuf>> mem_index_;
  std::unordered_map<std::string, uint64_t> lba_index_;
};
}  // namespace neodb
