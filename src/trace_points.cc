#include "trace_points.h"

namespace neodb {

using StatsMap = std::unordered_map<std::string, HistStats>;
std::map<std::thread::id, StatsMap> TracePoint::threaded_stats_map_;
std::vector<std::string> TracePoint::trace_point_names_;

// Return True IIF a new histogram instance was created.
bool TracePoint::GetOrCreateInstance(const std::string& name, HistStats** stats) {
  bool newly_created = false;
  auto& stats_map = threaded_stats_map_[std::this_thread::get_id()];
  auto it = stats_map.find(name);
  // If not present, we should create a new HistStats.
  if (it == stats_map.end()) {
    auto a = TimeUtils::GetCurrentTimeInUs();
    stats_map[name] = HistStats();
    auto b = TimeUtils::GetCurrentTimeInUs();
    LOG(INFO, "Construct new histogram for: {}, time cost: {}", name, b - a);
    // remember all the names, so we can reuse them in the future.
    trace_point_names_.emplace_back(name);
    newly_created = true;
  }
  *stats = &stats_map[name];
  return newly_created;
}

// Get all hist stats across all threads.
// TODO(KK.G) Need to add a second index to speed up name-based lookup.
std::vector<HistStats*> TracePoint::GetAllHistStats(const std::string& name) {
  std::vector<HistStats*> arr;
  for (auto& stats_map : threaded_stats_map_) {
    for (auto& item : stats_map.second) {
      if (item.first == name) {
        arr.push_back(&item.second);
      }
    }
  }
  return std::move(arr);
}

// Collect all histograms belong to the same name, across all threads.
HistStats TracePoint::GetMergedStats(const std::string& name) {
  auto arr = GetAllHistStats(name);
  HistStats merged;
  for (auto* stat : arr) {
    merged.Merge(*stat);
  }
  return std::move(merged);
}

// Collect all histograms of all names.
std::map<std::string, HistStats> TracePoint::GetAllMergedStats() {
  std::map<std::string, HistStats> merged_stats;
  for (const auto& name : trace_point_names_) {
    merged_stats[name] = GetMergedStats(name);
  }
  return std::move(merged_stats);
}

}  // namespace neodb