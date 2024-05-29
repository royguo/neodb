#pragma once
#include <unistd.h>

#include <atomic>

namespace neodb {
enum ZoneState { EMPTY, OPEN, FULL, OFFLINE };

class Zone {
 public:
  [[nodiscard]] uint64_t GetAvailableBytes() const {
    uint64_t used_bytes_ = (wp_ - offset_);
    return capacity_bytes_ - used_bytes_;
  }

  [[nodiscard]] uint64_t GetUsedBytes() const { return wp_ - offset_; }

 public:
  uint64_t id_ = 0;
  uint64_t offset_ = 0;
  uint64_t wp_ = 0;
  uint64_t capacity_bytes_ = 0;

  uint64_t expired_items_ = 0;
  uint64_t expired_bytes_ = 0;
  uint64_t total_items_ = 0;
  uint64_t total_pinned_items_ = 0;
  uint64_t finish_time_us_ = 0;

  ZoneState state_ = ZoneState::EMPTY;

  uint64_t open_time_us_ = 0;
  uint64_t close_time_us_ = 0;
};
}  // namespace neodb