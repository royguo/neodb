/**
 * TracePoint util is used for performance debugging, we should disable it in production.
 */
#pragma once
#include <unordered_map>

#include "neodb/histogram.h"
#include "utils.h"

#ifndef ENABLE_TRACE_POINT
#define TRACE_POINT(name, code)
#define PRINT_TRACE_POINT(name)
#define PRINT_ALL_TRACE_POINTS()
#else

#define TRACE_POINT(name, code)                                         \
  do {                                                                  \
    auto t1 = TimeUtils::GetCurrentTimeInUs();                          \
    code;                                                               \
    auto t2 = TimeUtils::GetCurrentTimeInUs();                          \
    HistStats* stats = nullptr;                                         \
    bool newly_created = TracePoint::GetOrCreateInstance(name, &stats); \
    auto t3 = TimeUtils::GetCurrentTimeInUs();                          \
    if (!newly_created) {                                               \
      stats->Append(t2 - t1);                                           \
    }                                                                   \
  } while (0)

#define PRINT_TRACE_POINT(name)                                   \
  do {                                                            \
    auto stats = TracePoint::GetMergedStats(name);                \
    LOG(INFO, "TRACE_POINT[{}]: {}", name, stats.ToString("ns")); \
  } while (0)

#define PRINT_ALL_TRACE_POINTS()                                         \
  do {                                                                   \
    LOG(INFO, "----- PRINT ALL TRACE POINTS -----");                     \
    auto merged_map = TracePoint::GetAllMergedStats();                   \
    for (auto& merged : merged_map) {                                    \
      LOG(INFO, "[{}]: {}", merged.first, merged.second.ToString("us")); \
    }                                                                    \
  } while (0)
#endif

namespace neodb {

// Use Histogram to trace performance bottlenecks.
class TracePoint {
 public:
  // @return True If a new instance was created, otherwise false.
  static bool GetOrCreateInstance(const std::string& name, HistStats** stats);

  // Collect all histograms belong to the same name, across all threads.
  static std::vector<HistStats*> GetAllHistStats(const std::string& name);

  // Get a merged stats across all threads with the same name.
  static HistStats GetMergedStats(const std::string& name);

  // Collect all stats that belong to the same name across all threads and merge them together
  static std::map<std::string, HistStats> GetAllMergedStats();

 private:
  using StatsMap = std::unordered_map<std::string, HistStats>;
  static std::map<std::thread::id, StatsMap> threaded_stats_map_;
  static std::vector<std::string> trace_point_names_;
};

}  // namespace neodb