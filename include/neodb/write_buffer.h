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
// A write buffer holds user written data, wait for flush.
class WriteBuffer {
 public:
  WriteBuffer(int capacity) : max_capacity_(capacity) {}

 public:
  Status PushOrWait(const std::string& key, std::shared_ptr<IOBuf> record);

  std::shared_ptr<IOBuf> PopOrWait();

 private:
  // Maximum capacity in bytes.
  uint64_t max_capacity_ = 0;

  // queue item: <key, record>
  std::queue<std::pair<std::string, std::shared_ptr<IOBuf>>> queue_;

  std::atomic<uint64_t> used_bytes_{0};

  std::mutex lk_;
  std::condition_variable cv_;
};

}  // namespace neodb
