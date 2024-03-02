#include "store.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "utils.h"

namespace neodb {
class StoreTest : public ::testing::Test {
 public:
  std::string device_prefix = "store_test_file_";
  std::unique_ptr<Store> store_;
  StoreOptions options_;

  void SetUp() override {
    options_.device_capacity_ = 1UL << 30;
    options_.device_path_ = CreateRandomFile(options_.device_capacity_);
    options_.device_zone_capacity_ = 256UL << 20;
    store_ = std::make_unique<Store>(options_);
  }

  void TearDown() override {
    int total = FileUtils::DeleteFilesByPrefix(".", device_prefix);
    LOG(INFO, "{} files was deleted", total);
  }

  std::string CreateRandomFile(uint32_t sz) {
    auto filename = FileUtils::GenerateRandomFile(device_prefix, sz);
    filenames_.emplace_back(filename);
    return filename;
  }

 private:
  std::vector<std::string> filenames_;
};

TEST_F(StoreTest, SingleWriteTest) {}
}  // namespace neodb