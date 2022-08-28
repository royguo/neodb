#include "neodb/write_buffer.h"

namespace neodb {

Status WriteBuffer::PushOrWait(const std::string& key,
                               std::shared_ptr<IOBuf> record) {
  int idx = 0;
  while (true) {
    // We only tries to lock inner queue which has enough space
    if (queues_[idx]->Size() < capacity_per_queue_ && queues_[idx]->TryLock()) {
      queues_[idx]->Emplace(key, record);
      queues_[idx]->Unlock();
      used_bytes_ += (key.size() + record->Size());
      // We don't need to hold lock here
      if (queues_[idx]->Size() >= capacity_per_queue_) {
        flush_cv_.notify_one();
      }
      break;
    }
    if (++idx == queue_num_) {
      idx = 0;
    }
  }
  return Status::OK();
}

void WriteBuffer::Flush(uint64_t queue_size_limit) {
  int idx = 0;
  std::unique_lock<std::mutex> lock(flush_lk_);
  flush_cv_.wait(lock, [&]() {
    while (idx < queue_num_) {
      // Find target queue
      if (queues_[idx]->Size() >=
          std::min(capacity_per_queue_, queue_size_limit)) {
        return true;
      }
      idx++;
    }
    return false;
  });

  queues_[idx]->Lock();
  auto pair = queues_[idx]->PopFront();
  used_bytes_ -= (pair.first.size() + pair.second->Size());
  flush_callback_(pair.first, 1001UL);
  // TODO flush data onto disk
  queues_[idx]->Unlock();
}
}
