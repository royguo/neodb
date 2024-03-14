#pragma once

#include <list>
#include <vector>

#include "neodb/io_buf.h"
#include "write_buffer.h"

#define IO_BLOCK_SIZE (32UL << 10)
#define CRC_SIZE 4

namespace neodb {
// Deprecated
class Codec {
 public:
  // pair: key -> lba
  using DataZoneKeyBuffer = std::list<std::pair<std::string, uint64_t>>;

  // Generate data zone's meta, which is flushed before data zone footer.
  // @param buffer Buffered key list of current data zone
  // @param meta Encoded meta info
  // @return encoded meta size, unaligned real size.
  static uint32_t GenerateDataZoneMeta(const DataZoneKeyBuffer& buffer,
                                       std::shared_ptr<IOBuf>& meta);

  // Decode the data zone's metadata and callback the related items one by one.
  static void DecodeDataZoneMeta(
      const char* meta_buffer, uint32_t buffer_size,
      const std::function<void(const std::string& key, uint64_t lba)>& cb);

  // @param meta_bytes real byte length of the meta data.
  static void GenerateDataZoneFooter(const DataZoneKeyBuffer& buffer,
                                     const std::shared_ptr<IOBuf>& buf,
                                     uint64_t meta_offset, uint32_t meta_bytes);

  static void DecodeDataZoneFooter(const std::shared_ptr<IOBuf>& footer_buf,
                                   uint64_t* meta_offset, uint32_t* meta_size);
};
}  // namespace neodb