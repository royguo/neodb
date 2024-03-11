#include "codec.h"

#include "logger.h"
#include "utils.h"
#include "write_buffer.h"

namespace neodb {

// Zone Meta:
// [key->lba items]
//    [key_len, 2B][lba 8B][key]
//    [key_len, 2B][lba 8B][key]
//    ...
// Zone Footer (not part of the zone meta):
// [4B footer, footer size, CRC, etc.]
//    ...
//    [key count 4B]
//    [footer offset 8B]
uint64_t Codec::GenerateDataZoneMeta(const Codec::DataZoneKeyBuffer& buffer,
                                     std::shared_ptr<IOBuf>& meta) {
  uint64_t size = 0;
  for (auto& pair : buffer) {
    size += (2 /* key len */ + 8 /* lba */ + pair.first.size());
  }
  // align to IO_PAGE_SIZE.
  size = (size + IO_PAGE_SIZE - 1) / IO_PAGE_SIZE * IO_PAGE_SIZE;
  assert(size > 0);
  // init new space for the metadata.
  meta = std::make_shared<IOBuf>(size);
  char* ptr = meta->Buffer();

  // Copy all items to the meta buffer.
  // pair: key -> lba
  // TODO(Roy Guo) Compression is nice to have.
  for (auto& item : buffer) {
    uint16_t key_len = item.first.size();
    memcpy(ptr, &key_len, 2);
    memcpy(ptr + 2, &(item.second), 8);
    memcpy(ptr + 10, item.first.data(), item.first.size());
    ptr += (10 + item.first.size());
  }
  meta->IncreaseSize(size);
  return NumberUtils::AlignTo(ptr - meta->Buffer(), IO_PAGE_SIZE);
}

void Codec::DecodeDataZoneMeta(
    const char* meta_buffer, uint32_t buffer_size,
    const std::function<void(const std::string&, uint64_t)>& cb) {
  assert(buffer_size > 0);
  const char* ptr = meta_buffer;
  while (ptr - meta_buffer < buffer_size) {
    uint16_t key_len = *reinterpret_cast<const uint16_t*>(ptr);
    uint64_t lba = *reinterpret_cast<const uint64_t*>(ptr + 2);
    std::string key = std::string(ptr + 10, key_len);
    cb(key, lba);
    ptr += (10 + key_len);
  }
}
}  // namespace neodb