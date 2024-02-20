#include "codec.h"

#include "write_buffer.h"

namespace neodb {
void Codec::EncodeWriteBuffer(std::unique_ptr<WriteBuffer> buffer, char* buf,
                              uint32_t* buffer_sz,
                              std::vector<uint32_t> item_offsets) {
  return;
}
}  // namespace neodb