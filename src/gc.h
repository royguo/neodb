#pragma once
#include <memory>

#include "zone.h"

namespace neodb {
class GC {
 public:
  enum WeightLabel { kZoneAge, kExpiredBytes };

 public:
  static std::shared_ptr<Zone> SelectGCCandidate(const std::vector<std::shared_ptr<Zone>>& zones);
};
}  // namespace neodb