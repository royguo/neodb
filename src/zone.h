#pragma once
#include <unistd.h>

namespace neodb {
struct Zone {
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

  // TODO
  uint64_t GetGCRank() { return expired_items_ / (total_items_ + 1); }

  friend bool operator<(Zone z1, Zone z2) {
    return z1.GetGCRank() > z2.GetGCRank();
  }
};
}  // namespace neodb