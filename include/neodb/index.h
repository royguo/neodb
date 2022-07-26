#pragma

#include "neodb/io_buf.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace neodb {
// Index's value is a uint64_t data pointer, the format is different between
// memory and SSD: [1]. Memory: shared_ptr<User Buffer> [2]. SSD:
//    64  =    43     +    12    +     7     +     2
//            lba         size      reserved     state
class Index {
 public:
  // If value is present in SSD, index point to the LBA address
  using SSDValuePtr = uint64_t;
  using MemoryValuePtr = std::shared_ptr<IOBuf>;
  using ValueType = std::variant<SSDValuePtr, MemoryValuePtr>;

 public:
  // The value is a pointer to the actual data, could be either in memory or
  // SSD.
  void Put(const std::string& key, ValueType value);

  ValueType Get(const std::string& key);

  bool UpdateIndex(const std::string& key, ValueType value);
};
}  // namespace neodb