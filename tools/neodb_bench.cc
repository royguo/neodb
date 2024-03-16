#include <memory>

#include "gflags/gflags.h"
#include "neodb/histogram.h"
#include "neodb/neodb.h"
#include "utils.h"

DEFINE_uint32(key_sz, 10, "key size of each item, default 10B");
DEFINE_uint32(value_sz, 100UL << 10, "value size of each item, default 100KB");
DEFINE_uint64(preload_size_gb, 5, "");
DEFINE_uint64(write_size_gb, 10, "total write GB for the test after preload");
DEFINE_uint32(read_ratio, 10, "read percent of total operations");
DEFINE_uint64(device_capacity_gb, 10, "");
DEFINE_uint64(zone_capacity_mb, 256, "");
DEFINE_uint32(workers, 1, "client thread number, default 1");

namespace neodb::tools {

// Benchmark workflow:
// Prepare:
//  Insert total a certain amount of data before run the tests.
// Run:
//  Read and write data.
class Benchmark {
 public:
  Benchmark() {
    StoreOptions store_options;
    store_options.device_path_ =
        FileUtils::GenerateRandomFile(device_prefix_, FLAGS_device_capacity_gb << 30);
    store_options.device_zone_capacity_ = FLAGS_zone_capacity_mb << 20;
    store_options.device_capacity_ = FLAGS_device_capacity_gb << 30;
    db_options_.store_options_list_.emplace_back(std::move(store_options));
    db_ = std::make_unique<NeoDB>(db_options_);
  }

  ~Benchmark() { FileUtils::DeleteFilesByPrefix(".", device_prefix_); }

  // The prepared data will not be remembered, just for occupying the space.
  void Prepare() {
    uint64_t preloaded_bytes = 0;
    uint64_t target_preload_bytes = (FLAGS_preload_size_gb << 30);
    uint64_t now = TimeUtils::GetCurrentTimeInSeconds();
    LOG(INFO, "Preloading start, target preload size: {}", target_preload_bytes);

    // For preloading, we don't need to generate different values at all.
    std::string common_value = StringUtils::GenerateRandomString(FLAGS_value_sz);
    while (preloaded_bytes < target_preload_bytes) {
      std::string key = StringUtils::GenerateRandomString(FLAGS_key_sz);
      preloaded_bytes += (key.size() + common_value.size());
      db_->Put(key, common_value);
      // Print each 5 seconds
      if (TimeUtils::GetCurrentTimeInSeconds() - now >= 5) {
        LOG(INFO, "Preload data size: {} MB", (preloaded_bytes >> 20));
        now = TimeUtils::GetCurrentTimeInSeconds();
      }
    }
    LOG(INFO, "Preloading finished, total written: {} bytes", preloaded_bytes);
  }

  void Run() {}

  void PrintStats() {}

 private:
  std::string device_prefix_ = "neodb_bench_file_";

  DBOptions db_options_;

  std::unique_ptr<NeoDB> db_;

  std::atomic<uint64_t> inserted_count_{0};
  std::atomic<uint64_t> inserted_bytes_{0};
};
}  // namespace neodb::tools

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
  neodb::tools::Benchmark benchmark;
  benchmark.Prepare();
  benchmark.Run();
  return 0;
}