#pragma

#include "neodb/io_buf.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace neodb {
/* Index's value is a uint64_t data pointer, the format is different between
 memory and SSD:
 [1]. Memory: shared_ptr<User Buffer>
 [2]. SSD:
    64  =    45     +      12         +     7
            lba         size hint        reserved

 This is how the size hint works:
  2  bits: page size of the hint (4KB/8KB/16KB/32KB)
  10 bits: page number of the

 The maximum hint size is (32KB*2^10 = 32MB), if the actual value size is
 larger, we could use the size in record header.
*/
class Index {
 public:
  // If value is present in SSD, index point to the LBA address
  using SSDValuePtr = uint64_t;
  using MemoryValuePtr = std::shared_ptr<IOBuf>;
  using ValueType = std::variant<SSDValuePtr, MemoryValuePtr>;

  // Index Flags
  // Indicate the returned value pointer is invalid(not found)
  static const uint64_t kIndexNotFound;

 public:
  // The value is a pointer to the actual data, could be either in memory or
  // SSD.
  void Put(const std::string& key, ValueType value);

  ValueType Get(const std::string& key);

  bool UpdateIndex(const std::string& key, ValueType value);

 private:
  std::map<std::string, std::variant<Index::SSDValuePtr, Index::MemoryValuePtr>>
      idx_;
};
}  // namespace neodb