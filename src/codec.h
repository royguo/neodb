#pragma once

#include <vector>

#include "neodb/io_buf.h"
#include "write_buffer.h"

#define CRC_BLOCK_SIZE (32UL << 10)
#define CRC_SIZE 4

namespace neodb {
class Codec {
 public:
  // Encode kv items to binary buffer.
  // Layout:
  // [item1]
  //      [key sz 2B]       <----- item offset
  //      [value sz 4B]
  //      [key]
  //      [value]
  // [item2]
  // [item3]
  //
  // Note that the CRC will be encoded into the buffer after each 32KB data.
  //
  // @param buffer Input immutable buffer
  // @param buf encoded binary buffer
  // @param buffer_sz The encoded buffer size
  // @param item_offsets The encoded item offsets starting from the buffer
  // offset.
  void EncodeWriteBuffer(std::unique_ptr<WriteBuffer> buffer, char* buf,
                         uint32_t* buffer_sz,
                         std::vector<uint32_t> item_offsets);

  // Decode a single item's key and value.
  // @param offset Global LBA offset of current item.
  void DecodeSingleItem(uint64_t offset, std::string* key,
                        std::shared_ptr<IOBuf> value);

  // Predict the encoded buffer size so we can allocate memory before the
  // encoding. CRC and alignment will be fulfilled during the process.
  // @param current_sz Total bytes of the existing encoded buffer.
  static uint32_t EstimateEncodedBufferSize(uint32_t current_sz,
                                            uint32_t key_sz, uint32_t value_sz);
};
}  // namespace neodb