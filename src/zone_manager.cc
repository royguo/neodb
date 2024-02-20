#include "zone_manager.h"

#include "codec.h"
#include "utils.h"

namespace neodb {
Status ZoneManager::Append(const std::string& key,
                           std::shared_ptr<IOBuf> value) {
  uint64_t hash = HashUtils::FastHash(key);
  auto& write_buffer = writable_buffers_[hash % options_.writable_buffer_num];
  return write_buffer->Put(key, value);
}

// Try to pick a immutable write buffer and encode, flush it to disk.
void ZoneManager::TryFlush() {
  std::unique_ptr<WriteBuffer> immutable;
  {
    std::unique_lock<std::mutex> lk(immutable_buffer_mtx_);
    immutable_buffer_cv_.wait(lk,
                              [&]() { return !immutable_buffers_.empty(); });

    // steal one immutable buffer
    immutable = std::move(immutable_buffers_.front());
    immutable_buffers_.pop_front();
  }

  // use the `immutable` buffer for further encoding and flushing.
  // TODO Encode
  Codec codec;
  codec.EncodeWriteBuffer(std::move(immutable), nullptr, nullptr, {});
  // TODO Flush
}

// void ZoneManager::EncodeItems(const ItemBuffer& items, char* buf,
//                               uint32_t buffer_sz,
//                               std::vector<uint32_t>& shift_offset) {
//   char* ptr = buf;
//   for (const auto& pair : items) {
//     const std::string& key = pair.first;
//     const std::shared_ptr<IOBuf>& value = pair.second;
//     // cache current encoded item's shift offset start from buffer beginning.
//     shift_offset.emplace_back((ptr - buf));
//     // Compress the target buffer and add CRC, return the encoded size.
//     uint32_t encoded_size = EncodeSingleItem(ptr, key.c_str(), key.size(),
//                                              value->Buffer(), value->Size());
//     ptr += encoded_size;
//   }
// }
//
// uint32_t ZoneManager::EncodeSingleItem(char* buf, uint32_t wp, const char*
// key,
//                                        uint16_t key_sz, const char* value,
//                                        uint32_t value_sz) {
//   // TODO(Roy Guo) Compress Key and Value Separately
//   // memcpy the compressed data into target buffer.
//   char* ptr = buf;
//   // The write pointer may start from non-zero position.
//   uint32_t block_shift_size = wp % EncodedBufferBlockSize;
//   // Skip CRC 4 bytes
//   if (wp % EncodedBufferBlockSize == 0) {
//     ptr += 4;
//   }
//   memcpy(ptr, &key_sz, 2);
//   ptr += 2;
//   memcpy(ptr, &value_sz, 4);
//   ptr += 4;
//
//   // encode key
//   uint16_t key_left = key_sz;
//   while (key_left > 0) {
//     // block free space = block size - newly written size - block_shift_size
//     uint16_t block_free_space =
//         EncodedBufferBlockSize - (ptr - buf) - block_shift_size;
//     // The free space can hold all the key bytes
//     if (key_left <= block_free_space) {
//       memcpy(ptr, key, key_left);
//       ptr += key_left;
//       key_left = 0;
//       break;
//     }
//     // If free space cannot hold all key bytes.
//     memcpy(ptr, key, block_free_space);
//     key_left = key_left - block_free_space;
//     ptr += block_free_space;
//   }
//
//   return 0;
// }
//
// void ZoneManager::DecodeSingleItem(const char* encoded_buf,
//                                    std::shared_ptr<IOBuf> key,
//                                    std::shared_ptr<IOBuf> value) {}
}  // namespace neodb