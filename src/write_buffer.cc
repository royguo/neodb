#include "neodb/write_buffer.h"

namespace neodb {

Status WriteBuffer::PushOrWait(const std::string& key,
                               std::shared_ptr<IOBuf> record) {
  std::unique_lock<std::mutex> lock(lk_);
  cv_.wait(lock, [&]() { return used_bytes_.load() < max_capacity_; });

  queue_.emplace(key, record);
  used_bytes_ += (key.size() + record->Size());

  return Status::OK();
}

void WriteBuffer::Flush() {
  // TODO only flush one items
  std::unique_lock<std::mutex> lock(lk_);
  cv_.wait(lock, [&]() { return queue_.size() > 0; });

  auto& pair = queue_.front();
  queue_.pop();

  used_bytes_ -= (pair.first.size() + pair.second->Size());
  flush_callback_(pair.first, 1001UL);
}
}
