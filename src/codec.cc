#include "codec.h"

#include "neodb/logger.h"
#include "write_buffer.h"

namespace neodb {

//uint32_t Codec::EstimateEncodedBufferSize(uint32_t current_sz, uint32_t key_sz,
//                                          uint32_t value_sz) {
//  // 2 bytes to represent key length
//  uint32_t key_len_bytes = 2;
//  // 4 bytes to represent value length
//  uint32_t value_len_bytes = 4;
//
//  uint32_t new_item_sz = key_sz + value_sz + key_len_bytes + value_len_bytes;
//  // padding space for to fulfill the current BLOCK
//  uint32_t padding_sz = current_sz % IO_BLOCK_SIZE > 0
//                            ? IO_BLOCK_SIZE - (current_sz % IO_BLOCK_SIZE)
//                            : 0;
//  // align current position to CRC_BLOCK_SIZE
//  if (padding_sz > 0 && new_item_sz >= padding_sz) {
//    new_item_sz -= padding_sz;
//    current_sz += padding_sz;
//    padding_sz = 0;
//  }
//  if (padding_sz == 0) {
//    // skip block header's CRC
//    current_sz += CRC_SIZE;
//  }
//
//  // add kv size and related CRC size
//  current_sz +=
//      (new_item_sz + new_item_sz / (CRC_BLOCK_SIZE - CRC_SIZE) * CRC_SIZE);
//
//  return current_sz;
//}
}  // namespace neodb