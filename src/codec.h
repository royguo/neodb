#pragma once

#include <vector>

#include "neodb/io_buf.h"
#include "write_buffer.h"

#define CRC_BLOCK_SIZE (32UL << 10)

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
  // @param buffer_sz encoded buffer size
  // @param item_offsets encoded item offsets.
  void EncodeWriteBuffer(std::unique_ptr<WriteBuffer> buffer, char* buf,
                         uint32_t* buffer_sz,
                         std::vector<uint32_t> item_offsets);
};
}  // namespace neodb