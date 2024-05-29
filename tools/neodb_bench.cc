#include <memory>

#include "gflags/gflags.h"
#include "logger.h"
#include "neodb/histogram.h"
#include "neodb/neodb.h"
#include "utils.h"

DEFINE_uint32(key_sz, 10, "key size of each item, default 10B");
DEFINE_uint32(value_sz, 100UL << 10, "value size of each item, default 100KB");
DEFINE_uint64(preload_size_gb, 1, "");
DEFINE_uint64(write_size_gb, 1, "total write GB for the test after preload");
DEFINE_uint32(read_ratio, 10, "read percent of total operations");
DEFINE_uint64(device_capacity_gb, 10, "");
DEFINE_uint64(zone_capacity_mb, 256, "");
DEFINE_uint32(workers, 2, "client thread number");

namespace neodb::tools {

// Benchmark workflow:
// Prepare:
//  Insert total a certain amount of data before run the tests.
// Run:
//  Read and write data.
class Benchmark {
 public:
  Benchmark() {
    LOG(INFO, "Benchmarking Initialized!");
    StoreOptions store_options1;
    store_options1.name_ = "store1";
    store_options1.device_path_ =
        FileUtils::GenerateRandomFile(device_prefix_, FLAGS_device_capacity_gb << 30);
    store_options1.device_zone_capacity_ = FLAGS_zone_capacity_mb << 20;
    store_options1.device_capacity_ = FLAGS_device_capacity_gb << 30;

    //    StoreOptions store_options2;
    //    store_options2.name_ = "store2";
    //    store_options2.device_path_ =
    //        FileUtils::GenerateRandomFile(device_prefix_, FLAGS_device_capacity_gb << 30);
    //    store_options2.device_zone_capacity_ = FLAGS_zone_capacity_mb << 20;
    //    store_options2.device_capacity_ = FLAGS_device_capacity_gb << 30;

    // init database with two devices.
    db_options_.store_options_list_.emplace_back(std::move(store_options1));
    //    db_options_.store_options_list_.emplace_back(std::move(store_options2));
    db_ = std::make_unique<NeoDB>(db_options_);
  }

  ~Benchmark() { FileUtils::DeleteFilesByPrefix(".", device_prefix_); }

  // The prepared data will not be remembered, just for occupying the space.
  void Prefill() {
    if (FLAGS_preload_size_gb == 0) {
      LOG(INFO, "Skip preload");
      return;
    }
    uint64_t preloaded_bytes = 0;
    uint64_t target_preload_bytes = (FLAGS_preload_size_gb << 30);
    uint64_t now = TimeUtils::GetCurrentTimeInSeconds();
    LOG(INFO, "Preloading start, target preload size: {}", target_preload_bytes);

    // For preloading, we don't need to generate different values at all.
    std::string common_value = StringUtils::GenerateRandomString(FLAGS_value_sz);
    while (preloaded_bytes < target_preload_bytes) {
      std::string key = StringUtils::GenerateRandomString(FLAGS_key_sz);
      preloaded_bytes += (key.size() + common_value.size());
      auto s = db_->Put(key, common_value);
      if (!s.ok()) {
        LOG(ERROR, "Put failed: {}", s.msg());
      }
      // Print each 5 seconds
      if (TimeUtils::GetCurrentTimeInSeconds() - now >= 5) {
        LOG(INFO, "Preload data size: {} MB", (preloaded_bytes >> 20));
        now = TimeUtils::GetCurrentTimeInSeconds();
      }
    }
    LOG(INFO, "Preloading finished, total written: {} bytes", preloaded_bytes);
  }

  void Verify() {
    // Verify data
    //    uint64_t hits = 0;
    //    uint64_t not_found = 0;
    //    uint64_t corrupts = 0;
    //
    //    for (auto& item : verify_keys_) {
    //      auto keys_arr = item.second;
    //      for (auto& key : keys_arr) {
    //        std::string value;
    //        auto s = db_->Get(key, &value);
    //        if (s.code() == Status::Code::kNotFound) {
    //          not_found++;
    //          continue;
    //        }
    //        assert(s.ok());
    //        if (value == common_value) {
    //          hits++;
    //        } else {
    //          corrupts++;
    //        }
    //      }
    //    }
    //    LOG(INFO, "Total items: {}, hits: {}, not_found: {}, hit rate: {}%, corrupts: {}",
    //    total_item,
    //        hits, not_found, hits * 100.0 / total_item, corrupts);
  }

  void Run() {
    std::atomic<uint64_t> total_item = 0;
    std::vector<HistStats> write_stats_;
    uint64_t target_write_bytes = FLAGS_write_size_gb << 30;
    std::atomic<uint64_t> written_bytes = 0;
    uint64_t item_cnt_per_worker =
        target_write_bytes / (FLAGS_key_sz + FLAGS_value_sz) / FLAGS_workers;
    assert(item_cnt_per_worker > 0);
    LOG(INFO, "item_cnt_per_worker: {}", item_cnt_per_worker);

    std::string common_value = StringUtils::GenerateRandomString(FLAGS_value_sz);

    // Put key values in to the store.
    auto start = TimeUtils::GetCurrentTimeInMs();
    for (int i = 0; i < FLAGS_workers; ++i) {
      write_stats_.emplace_back();
      workers_.emplace_back([&, i]() {
        // Write data
        while (written_bytes < target_write_bytes) {
          std::string key = StringUtils::GenerateRandomString(FLAGS_key_sz);
          auto t1 = TimeUtils::GetCurrentTimeInUs();
          auto s = db_->Put(key, common_value);
          assert(s.ok());
          write_stats_[i].Append(TimeUtils::GetCurrentTimeInUs() - t1);
          written_bytes += (key.size() + common_value.size());
          verify_keys_[std::this_thread::get_id()].emplace_back(std::move(key));
          total_item++;
        }
      });
    }

    for (auto& worker : workers_) {
      worker.join();
    }
    db_ = nullptr;

    auto end = TimeUtils::GetCurrentTimeInMs();
    uint64_t throughput_mb = (target_write_bytes / ((end - start) / 1000)) >> 20;

    LOG(INFO, "Finish Benchmarking Insertion");

    HistStats stats;
    for (auto& stat : write_stats_) {
      stats.Merge(stat);
    }

    std::string line = "--------------------------------------------------------";
    LOG(INFO, line);
    LOG(INFO, "Time cost: {} ms, total bytes written: {}, throughput: {} MB/s", end - start,
        target_write_bytes, throughput_mb);
    LOG(INFO, line);
    LOG(INFO, "{}", stats.ToString(" us"));
    LOG(INFO, line);
  }

 private:
  void DEBUG_PrintIndexValue(const std::string& key) {
    auto index = db_->DEBUG_GetIndex(0);
    Index::ValueVariant value;
    index->Get(key, value);
    if (std::holds_alternative<Index::MemValue>(value)) {
      LOG(INFO, "key :{}, index points to memory value", key);
    } else {
      uint64_t lba = std::get<Index::LBAValue>(value);
      LOG(INFO, "key: {}, index points to lba value: {}", key, lba);
    }
  }

 private:
  std::string device_prefix_ = "neodb_bench_file_";

  DBOptions db_options_;

  std::unique_ptr<NeoDB> db_;

  std::map<std::thread::id, std::vector<std::string>> verify_keys_;

  std::vector<std::thread> workers_;
};
}  // namespace neodb::tools

using namespace neodb;

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  LOG(INFO, "Benchmark initialized, param:");
  LOG(INFO,
      "\n"
      "key_sz: {} bytes\n"
      "value_sz: {} bytes\n"
      "device size: {} GB\n"
      "zone size: {} MB\n"
      "preload size: {} GB\n"
      "write size: {} GB\n"
      "read ratio: {}%",
      FLAGS_key_sz, FLAGS_value_sz, FLAGS_device_capacity_gb, FLAGS_zone_capacity_mb,
      FLAGS_preload_size_gb, FLAGS_write_size_gb, FLAGS_read_ratio);
  neodb::InitLogger();
  neodb::tools::Benchmark benchmark;
  benchmark.Prefill();
  benchmark.Run();
  PRINT_ALL_TRACE_POINTS();
  return 0;
}