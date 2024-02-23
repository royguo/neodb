#pragma once

#include <vector>

#include "neodb/io_buf.h"
#include "write_buffer.h"

#define IO_BLOCK_SIZE (32UL << 10)
#define CRC_SIZE 4

namespace neodb {
// Deprecated
class Codec {
 public:
};
}  // namespace neodb