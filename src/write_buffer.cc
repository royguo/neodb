#include "write_buffer.h"

#include "codec.h"
#include "logger.h"

namespace neodb {
Status WriteBuffer::Put(const std::string& key,
                        const std::shared_ptr<IOBuf>& value) {
  if (immutable) {
    return Status::StatusError(
        "Put failed, WriteBuffer is already FULL and immutable!");
  }
  items_.emplace_back(key, value);
  used_bytes_ += (key.size() + value->Size());
  // If the capacity is exceeded, we should change this buffer to immutable.
  if (used_bytes_ >= capacity_bytes_) {
    LOG(DEBUG, "Convert to immutable write buffer, used_bytes: {}",
        used_bytes_);
    immutable = true;
  }
  return Status::OK();
}
}  // namespace neodb