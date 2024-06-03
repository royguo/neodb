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
// @return encoded meta size, unaligned real size
uint32_t Codec::GenerateDataZoneMeta(const Codec::DataZoneKeyBuffer& buffer,
                                     std::shared_ptr<IOBuf>& meta) {
  uint64_t capacity = 0;
  for (auto& pair : buffer) {
    capacity += (2 /* key len */ + 8 /* lba */ + pair.first.size());
  }
  // Align to IO_PAGE_SIZE.
  capacity = (capacity + IO_PAGE_SIZE - 1) / IO_PAGE_SIZE * IO_PAGE_SIZE;
  assert(capacity > 0);
  // init new space for the metadata.
  meta = std::make_shared<IOBuf>(capacity);
  char* ptr = meta->Buffer();

  // Copy all items to the meta buffer.
  // pair: key -> lba
  // TODO(Roy Guo) Compression is nice to have.
  uint32_t key_cnt = 0;
  for (auto& item : buffer) {
    uint16_t key_len = item.first.size();
    memcpy(ptr, &key_len, 2);
    memcpy(ptr + 2, &(item.second), 8);
    memcpy(ptr + 10, item.first.data(), key_len);
    ptr += (10 + item.first.size());
    key_cnt++;
  }
  meta->IncreaseSize(capacity);
  // return the real size
  return ptr - meta->Buffer();
}

// TODO check CRC
void Codec::DecodeDataZoneMeta(const char* meta_buffer, uint32_t buffer_size,
                               const std::function<void(const std::string&, uint64_t)>& cb) {
  assert(buffer_size > 0);
  const char* ptr = meta_buffer;
  uint32_t key_cnt = 0;
  while (ptr - meta_buffer < buffer_size) {
    uint16_t key_len = *reinterpret_cast<const uint16_t*>(ptr);
    uint64_t lba = *reinterpret_cast<const uint64_t*>(ptr + 2);
    std::string key = std::string(ptr + 10, key_len);
    cb(key, lba);
    ptr += (10 + key_len);
    key_cnt++;
  }
  LOG(INFO, "Decode data zone meta, key number: {} ", key_cnt);
}

// Footer:
// [...]
// [key count 4B]
// [meta total size 4B]
// [meta offset 8B] <--- zone end
// @param meta_bytes Real un-aligned buffer size.
void Codec::EncodeDataZoneFooter(const Codec::DataZoneKeyBuffer& buffer,
                                 const std::shared_ptr<IOBuf>& buf, uint64_t meta_offset,
                                 uint32_t meta_bytes) {
  assert(buf->Capacity() % IO_PAGE_SIZE == 0);
  assert(buf->Capacity() >= IO_PAGE_SIZE);
  uint32_t key_cnt = buffer.size();
  //  buf->AppendZeros(buf->Capacity() - 16);
  // Skip padding 0 zeros
  buf->IncreaseSize(buf->Capacity() - 16);
  buf->Append((char*)&key_cnt, 4);
  buf->Append((char*)&meta_bytes, 4);
  buf->Append((char*)&meta_offset, 8);
}

void Codec::DecodeDataZoneFooter(const std::shared_ptr<IOBuf>& footer_buf, uint64_t* meta_offset,
                                 uint32_t* meta_size) {
  char* ptr = footer_buf->Buffer();
  *meta_offset = *reinterpret_cast<uint64_t*>(ptr + IO_PAGE_SIZE - 8);
  *meta_size = *reinterpret_cast<uint32_t*>(ptr + IO_PAGE_SIZE - 12);
}

}  // namespace neodb