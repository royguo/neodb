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
  void SetUp() override { InitLogger(); }

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

TEST_F(NeoDBTest, SimplePutAndGetTest) {
  DBOptions db_options;
  StoreOptions store_options;

  store_options.device_capacity_ = 500UL << 20;
  store_options.device_path_ = CreateRandomFile(store_options.device_capacity_);
  store_options.device_zone_capacity_ = 100UL << 20;

  db_options.store_options_list_.push_back(store_options);
  NeoDB db(db_options);

  db.Put("test_key", "test_value");
  std::string value;
  db.Get("test_key", &value);
  EXPECT_EQ("test_value", value);
}

TEST_F(NeoDBTest, LargeAmountPutAndGetTest) {
  DBOptions db_options;
  StoreOptions store_options;

  store_options.device_capacity_ = 100UL << 20;
  store_options.device_path_ = CreateRandomFile(store_options.device_capacity_);
  store_options.device_zone_capacity_ = 10UL << 20;

  db_options.store_options_list_.push_back(store_options);
  NeoDB db(db_options);

  std::map<std::string, std::string> source;
  uint64_t total_bytes = 0;
  while (total_bytes < (300UL << 20)) {
    std::string key = StringUtils::GenerateRandomString(10);
    std::string value = StringUtils::GenerateRandomString(100UL << 10);
    db.Put(key, value);
    source.emplace(key, value);
    total_bytes += (key.size() + value.size());
  }

  LOG(INFO, "data insertion finished, total : {}", source.size());

  // Check Existence
  uint32_t hits = 0;
  uint32_t total = source.size();
  uint32_t not_found = 0;
  for (auto& item : source) {
    std::string value;
    auto s = db.Get(item.first, &value);
    if (s.code() == Status::Code::kNotFound) {
      not_found++;
      continue;
    }
    EXPECT_TRUE(s.ok());

    if (item.second == value) {
      hits++;
    }
  }
  EXPECT_GT(hits, not_found);
  LOG(INFO, "hits: {}, not_found: {}, total: {}, hit rate: {}", hits, not_found, total,
      hits * 100 / total);
}
}  // namespace neodb