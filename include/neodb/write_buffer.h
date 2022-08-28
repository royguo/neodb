#pragma once

#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "neodb/index.h"
#include "neodb/status.h"

namespace neodb {

class WriteBuffer {
 public:
  using FlushCallback = std::function<void(const std::string&, uint64_t)>;
  using QueueItem = std::pair<std::string, std::shared_ptr<IOBuf>>;

 public:
  class BufferQueue {
   public:
    BufferQueue() {}
    ~BufferQueue() {}
    bool TryLock() { return lock_.try_lock(); }

    void Lock() { lock_.lock(); }

    void Unlock() { lock_.unlock(); }

    void Emplace(const std::string& key, std::shared_ptr<IOBuf> record) {
      queue_.emplace(key, record);
      used_bytes_ += key.size() + record->Size();
    }

    QueueItem PopFront() {
      auto& item = queue_.front();
      return std::move(item);
    }

    uint64_t Size() { return used_bytes_; }

   private:
    std::queue<QueueItem> queue_;
    std::mutex lock_;
    uint64_t used_bytes_ = 0;
  };

 public:
  WriteBuffer(uint64_t max_capacity, int queue_num,
              const FlushCallback& callback)
      : queue_num_(queue_num),
        flush_callback_(callback),
        max_capacity_(max_capacity) {
    capacity_per_queue_ = max_capacity_ / queue_num_;
    assert(queue_num > 0);
    assert(capacity_per_queue_ > 0);

    for (int i = 0; i < queue_num; ++i) {
      queues_.emplace_back(std::make_unique<BufferQueue>());
    }
  }

 public:
  Status PushOrWait(const std::string& key, std::shared_ptr<IOBuf> record);

  // Flush write buffer on to disk and triggers flush callback;
  // @param queue_size_limit Customize the size limit of current flush, default
  // 10GB. the flush thread will use min(queue_size_limit, capacity_per_queue_)
  // to decide the flush throttle.
  void Flush(uint64_t queue_size_limit = 10ULL << 30 /*1GB*/);

  uint64_t Capacity() { return max_capacity_; }

 private:
  // queue item: <key, record>
  std::vector<std::unique_ptr<BufferQueue>> queues_;

  // Total number of underlying queues
  int queue_num_ = 3;

  // Triggered after each item flushed onto disk.
  FlushCallback flush_callback_;

  // Maximum capacity in bytes.
  uint64_t max_capacity_ = 0;

  // max_capacity / queue_num
  uint64_t capacity_per_queue_ = 0;

  std::atomic<uint64_t> used_bytes_{0};

  std::mutex flush_lk_;
  std::condition_variable flush_cv_;
};

}  // namespace neodb
