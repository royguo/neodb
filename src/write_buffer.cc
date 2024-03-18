#include "write_buffer.h"

#include "codec.h"
#include "logger.h"

namespace neodb {

// TODO (Roy Guo) Change to blocking write if the buffer is FULL.
Status WriteBuffer::Put(const std::string& key, const std::shared_ptr<IOBuf>& value) {
  if (immutable_) {
    return Status::StatusError("Put failed, WriteBuffer is already FULL and immutable_!");
  }
  items_.emplace_back(key, value);
  used_bytes_ += (key.size() + value->Size());
  // If the capacity is exceeded, we should change this buffer to immutable_.
  if (used_bytes_ >= capacity_bytes_) {
    LOG(DEBUG, "Convert to immutable_ write buffer, used_bytes: {}", used_bytes_);
    immutable_ = true;
  }
  return Status::OK();
}
}  // namespace neodb