#pragma once
#include <unistd.h>

#include <atomic>

namespace neodb {
enum ZoneState { EMPTY, OPEN, FULL };
class Zone {
 public:
  // TODO
  uint64_t GetGCRank() { return expired_items_ / (total_items_ + 1); }

  friend bool operator<(Zone z1, Zone z2) {
    return z1.GetGCRank() > z2.GetGCRank();
  }

  // Lock the zone for data writing
  bool Acquire() { return write_lock_.try_lock(); }

  // Unlock the zone if the zone was already FULL.
  void Release() { write_lock_.unlock(); }

 public:
  uint64_t id_;
  uint64_t offset_;
  uint64_t wp_;
  uint64_t used_bytes_;
  uint64_t capacity_bytes_;

  uint64_t expired_items_;
  uint64_t expired_bytes_;
  uint64_t total_items_;
  uint64_t total_pinned_items_;
  uint64_t finish_time_us_;

  ZoneState state_ = ZoneState::EMPTY;

 private:
  std::mutex write_lock_;
};
}  // namespace neodb