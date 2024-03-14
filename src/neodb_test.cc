#include "neodb/neodb.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "store.h"
#include "utils.h"

namespace neodb {
class NeoDBTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {
    int total = FileUtils::DeleteFilesByPrefix(".", device_prefix);
    LOG(INFO, "{} files was deleted successfully", total);
  }

  std::string CreateRandomFile(uint32_t sz) {
    auto filename = FileUtils::GenerateRandomFile(device_prefix, sz);
    filenames_.emplace_back(filename);
    return filename;
  }

 private:
  std::string device_prefix = "neodb_test_file_";
  std::vector<std::string> filenames_;
};

TEST_F(NeoDBTest, StoreInitTest) {
  DBOptions db_options;
  StoreOptions store_options;

  store_options.device_capacity_ = 500UL << 20;
  store_options.device_path_ = CreateRandomFile(store_options.device_capacity_);
  store_options.device_zone_capacity_ = 100UL << 20;

  db_options.store_options_list_.push_back(store_options);
  NeoDB neo_db(db_options);
}
}  // namespace neodb