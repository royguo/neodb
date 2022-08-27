#include "neodb/write_buffer.h"

namespace neodb {

Status WriteBuffer::PushOrWait(const std::string& key,
                               std::shared_ptr<IOBuf> record) {
  std::unique_lock<std::mutex> lock(lk_);
  cv_.wait(lock, [&]() { return used_bytes_.load() < max_capacity_; });

  queue_.emplace(std::make_pair<>(key, record));
  used_bytes_ += (key.size() + record->Size());

  return Status::OK();
}

std::shared_ptr<IOBuf> PopOrWait() {}
}
