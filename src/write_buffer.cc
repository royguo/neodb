#include "write_buffer.h"

#include "codec.h"

namespace neodb {
Status WriteBuffer::Put(const std::string& key, std::shared_ptr<IOBuf> value) {
  if (immutable) {
    return Status::StatusError(
        "Put failed, WriteBuffer already changed to immutable!");
  }
  std::unique_lock<std::mutex> lk(mtx_);
  // Increase expected encoded buffer size for the newly added kv pair.
  expected_encoded_sz_ = Codec::EstimateEncodedBufferSize(
      expected_encoded_sz_, key.size(), value->Size());
  items_.emplace_back(key, value);
  used_bytes_ += (key.size() + value->Size());
  // If the capacity is exceeded, we should change this buffer to immutable.
  if (used_bytes_ >= capacity_bytes_) {
    immutable = true;
  }
  return Status::OK();
}
}  // namespace neodb