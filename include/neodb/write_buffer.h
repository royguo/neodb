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

 public:
  WriteBuffer(uint64_t capacity, const FlushCallback& callback)
      : flush_callback_(callback), max_capacity_(capacity) {}

 public:
  Status PushOrWait(const std::string& key, std::shared_ptr<IOBuf> record);

  // Flush write buffer on to disk and triggers flush callback;
  void Flush();

  uint64_t Capacity() { return max_capacity_; }

 private:
  // queue item: <key, record>
  std::queue<std::pair<std::string, std::shared_ptr<IOBuf>>> queue_;

  FlushCallback flush_callback_;

  // Maximum capacity in bytes.
  uint64_t max_capacity_ = 0;

  std::atomic<uint64_t> used_bytes_{0};

  std::mutex lk_;
  std::condition_variable cv_;
};

}  // namespace neodb
