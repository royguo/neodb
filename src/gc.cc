#include "gc.h"

#include <map>
#include <vector>

#include "utils.h"
#include "zone.h"

namespace neodb {
std::shared_ptr<Zone> GC::SelectGCCandidate(const std::vector<std::shared_ptr<Zone>>& zones) {
  // Default label weights
  static std::map<WeightLabel, int> weights{{WeightLabel::kZoneAge, 50},
                                            {WeightLabel::kExpiredBytes, 50}};

  // Collect necessary stat info.
  uint64_t now = TimeUtils::GetCurrentTimeInUs();
  uint64_t oldest_finish_age = 0;
  uint64_t max_expired_bytes = 0;
  for (const auto& zone : zones) {
    oldest_finish_age = std::max(oldest_finish_age, (now - zone->finish_time_us_));
    max_expired_bytes = std::max(zone->expired_bytes_, max_expired_bytes);
  }

  // Loop calculate scores of each zone.
  uint64_t max_score = 0;
  std::shared_ptr<Zone> target;
  for (const auto& zone : zones) {
    // Only FULL zone should be considered as candidate.
    if (zone->state_ != ZoneState::FULL) {
      continue;
    }
    uint64_t zone_age_score =
        (now - zone->finish_time_us_) * weights[kZoneAge] / (oldest_finish_age + 1) / 100;
    uint64_t expired_bytes_score =
        (zone->expired_bytes_ * weights[kExpiredBytes]) / (max_expired_bytes + 1) / 100;
    uint64_t score = zone_age_score + expired_bytes_score;
    if (score >= max_score) {
      max_score = score;
      target = zone;
    }
  }
  LOG(INFO, "GC target selected, score: {}, zone id : {}", max_score, target->id_);
  return target;
}
}  // namespace neodb
