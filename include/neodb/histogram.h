#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logger.h"

#ifndef ENABLE_TRACE_POINT
#define TRACE_POINT(name, code)
#define PRINT_TRACE_POINT(name)
#define PRINT_ALL_TRACE_POINTS()
#else

#define TRACE_POINT(name, code)                       \
  do {                                                \
    auto t1 = TimeUtils::GetCurrentTimeInUs();        \
    code;                                             \
    auto t2 = TimeUtils::GetCurrentTimeInUs();        \
    HistStats* stats = TracePoint::GetInstance(name); \
    stats->Append(t2 - t1);                           \
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
/*
 * NOT THREAD SAFE.
 *
 * The histogram of each operation, note that this class is not thread-safe.
 * Simple Usage:
 * HistStats stats;
 * for (int i = 0; i < 100; ++i) {
 *   auto begin_time = Now();
 *   ...
 *   auto end_time = Now();
 *   stats.append(end_time - begin_time);
 * }
 * std::cout << stats.result_string();
 */
class HistStats {
 public:
  // Init the bucket on initialization.
  HistStats() : buckets_(bucket_size_, 0) {}

  // Append execution time of a record
  void Append(size_t unit) {
    if (unit < buckets_.size()) {
      ++buckets_[unit];
    } else {
      large_nums_.emplace_back(unit);
    }
    total_count_++;
  }

  // Merge two Histogram.
  void Merge(const HistStats& another) {
    large_nums_.insert(large_nums_.end(), another.large_nums_.begin(), another.large_nums_.end());
    for (size_t d = 0; d < another.buckets_.size(); ++d) {
      size_t c = another.buckets_[d];
      buckets_[d] += c;
    }
    total_count_ += another.ElementCount();
  }

  [[nodiscard]] uint64_t ElementCount() const { return total_count_; }

  void Reset() {
    std::fill(buckets_.begin(), buckets_.end(), 0);
    large_nums_.resize(0);
    if (large_nums_.capacity() > (10 << 10)) {
      large_nums_.shrink_to_fit();
    }
    total_count_ = 0;
  }

  std::string ToString(const char* unit_suffix = "ns") {
    auto result = getResult({0.50, 0.90, 0.95, 0.99, 0.999});
    char buf[1024];
    int len = snprintf(buf, sizeof(buf),
                       "P50:%zu%s P90:%zu%s P95:%zu%s P99:%zu%s P999:%zu%s "
                       "Avg:%zu%s Max:%zu%s",
                       result[0], unit_suffix, result[1], unit_suffix, result[2], unit_suffix,
                       result[3], unit_suffix, result[4], unit_suffix, result[result.size() - 2],
                       unit_suffix, result[result.size() - 1], unit_suffix);
    return {buf, static_cast<size_t>(len)};
  }

 private:
  // Get latency results(with avg and max result).
  // e.g. get_result({0.5, 0.9, 0.99}) will return [p50, p90, p99, avg, max]
  template <size_t N>
  auto getResult(const double (&percentiles)[N]) -> std::array<size_t, N + 2> {
    assert(std::is_sorted(std::begin(percentiles), std::end(percentiles)));
    double reciprocal_total =
        1.0 / (std::accumulate(buckets_.cbegin(), buckets_.cend(), 0) + large_nums_.size());

    std::array<size_t, N + 2> result{};
    size_t idx = 0;
    size_t accum = 0;
    size_t total_tm = 0;
    size_t& avg = result[N];
    size_t& max = result[N + 1];

    for (size_t d = 0; d < buckets_.size(); ++d) {
      size_t c = buckets_[d];
      if (c) {
        accum += c;
        total_tm += d * c;
        while (idx < N && accum * reciprocal_total >= percentiles[idx]) {
          result[idx++] = d;
        }
        max = d;
      }
    }

    if (!large_nums_.empty()) {
      std::sort(large_nums_.begin(), large_nums_.end());
      for (unsigned long& large_num : large_nums_) {
        ++accum;
        total_tm += large_num;
        while (idx < N && accum * reciprocal_total >= percentiles[idx]) {
          result[idx++] = large_num;
        }
      }
      max = large_nums_.back();
    }
    avg = total_tm / std::max<size_t>(1, accum);
    return result;
  }

 private:
  // Total operation count
  uint64_t total_count_ = 0;

  // Total bucket size, each bucket slot contains a
  // exact nanoseconds, so the range is [0ns, 10ms]
  uint64_t bucket_size_ = (10 << 20);

  // Space for each histgram (duration from nanoseconds to microseconds)
  // Pre-allocated on initialization.
  std::vector<uint64_t> buckets_ = {};

  // If the latency exceeds `buckets_.size()` we don't need to expand the bucket
  // size, here we just append all large latencies to `large_nums`.
  // e.g. sometimes we get 1 seconds max latency, we have no reason to make the
  // bucket size `1*1000*1000*1000`(unit).
  std::vector<size_t> large_nums_;
};

// Use Histogram to trace performance bottlenecks.
class TracePoint {
 public:
  // Global container of all trace points.
  static std::map<std::string, HistStats>& GetSharedStatsMap() {
    // key: combination of point name and thread id.
    static std::map<std::string, HistStats> stats_map;
    return stats_map;
  }

  static std::vector<std::string>& GetSharedPointNames() {
    static std::vector<std::string> names;
    return names;
  }

  static HistStats* GetInstance(const std::string& name) {
    auto& stats_map = GetSharedStatsMap();
    auto key = GetTracePointKey(name);
    auto it = stats_map.find(key);
    // If not present, we should create a new HistStats.
    if (it == stats_map.end()) {
      if (!IsValidTracePointKey(name)) {
        LOG(ERROR, "TracePoint name \"{}\" is not valid, shouldn't be a prefix of existing names.",
            name);
        return nullptr;
      }
      stats_map[key] = HistStats();
      LOG(INFO, "Add new histogram trace point name: {}", name);
      // remember all the names, so we can reuse them in the future.
      GetSharedPointNames().emplace_back(name);
    }
    return &stats_map[key];
  }

  static std::string GetTracePointKey(const std::string& name) {
    std::ostringstream ss;
    ss << name;
    ss << std::this_thread::get_id();
    return ss.str();
  }

  // Collect all histograms belong to the same name, across all threads.
  static std::vector<HistStats*> GetAllHistStats(const std::string& name) {
    auto& stats_map = GetSharedStatsMap();
    std::vector<HistStats*> arr;
    for (auto& item : stats_map) {
      // Find by name as prefix (the suffix is thread ids)
      if (item.first.find(name) == 0) {
        arr.push_back(&item.second);
      }
    }
    return std::move(arr);
  }

  static HistStats GetMergedStats(const std::string& name) {
    auto arr = GetAllHistStats(name);
    HistStats merged;
    for (auto* stat : arr) {
      merged.Merge(*stat);
    }
    return std::move(merged);
  }

  static std::map<std::string, HistStats> GetAllMergedStats() {
    std::map<std::string, HistStats> merged_stats;
    auto& names = GetSharedPointNames();
    for (const auto& name : names) {
      merged_stats[name] = GetMergedStats(name);
    }
    return std::move(merged_stats);
  }

  // A trace point's name should NOT be a prefix of an existing name
  static bool IsValidTracePointKey(const std::string& name) {
    auto& existing_names = GetSharedPointNames();
    for (auto& existing_name : existing_names) {
      if (existing_name.find(name) == 0 && existing_name.size() != name.size()) {
        LOG(INFO, "new name: {}, existing name: {}", name, existing_name);
        return false;
      }
    }
    return true;
  }
};

}  // namespace neodb