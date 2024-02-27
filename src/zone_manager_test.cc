#include "zone_manager.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "utils.h"

namespace neodb {
class ZoneManagerTest : public ::testing::Test {
 public:
  std::string filename_;
  ZoneManager* zone_manager_;
  DBOptions options_;

  void SetUp() override {
    filename_ =
        FileUtils::GenerateRandomFile("zone_manager_test_file_", 1UL << 30);
    auto io_handle = std::make_unique<FileIOHandle>(filename_);
    zone_manager_ = new ZoneManager(options_, std::move(io_handle));
  }

  void TearDown() override {
    delete zone_manager_;
    FileUtils::DeleteFile(filename_);
  }
};

TEST_F(ZoneManagerTest, ProcessSingleItemTest) {
  // IO flush size set to 32KB
  std::shared_ptr<IOBuf> buffer = std::make_shared<IOBuf>(32UL << 10);
  char* ptr = buffer->Buffer();

  std::string key = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(10UL << 10));

  bool reminding = zone_manager_->ProcessSingleItem(
      buffer, key, value, [](uint64_t lba) { EXPECT_EQ(lba, 0); });

  // total size = 6 + 10 + 10KB
  EXPECT_TRUE(reminding);
  EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
  EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
  EXPECT_EQ(key, std::string(ptr + 6, key.size()));
  EXPECT_EQ(value->Data(), std::string(ptr + 6 + key.size(), value->Size()));
  ptr += 6 + 10 + (10UL << 10);

  // total size = 6 + 10 + 10KB + 6 + 2
  key = StringUtils::GenerateRandomString(1);
  value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(1));
  reminding = zone_manager_->ProcessSingleItem(
      buffer, key, value,
      [](uint64_t lba) { EXPECT_EQ(lba, (10UL << 10) + 16); });

  EXPECT_TRUE(reminding);
  EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
  EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
  EXPECT_EQ(key, std::string(ptr + 6, key.size()));
  EXPECT_EQ(value->Data(), std::string(ptr + 6 + key.size(), value->Size()));
  ptr += 6 + 1 + 1;

  // total size = 6 + 10 + 10KB + 6 + 2 + 6 + 10 + 22KB
  key = StringUtils::GenerateRandomString(10);
  value =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(22UL << 10));
  reminding =
      zone_manager_->ProcessSingleItem(buffer, key, value, [&](uint64_t lba) {
        LOG(INFO, "item flushed");
        EXPECT_EQ(lba, (10UL << 10) + 16 + 6 + 2);
        EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
        EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
        EXPECT_EQ(key, std::string(ptr + 6, key.size()));
      });
  EXPECT_TRUE(reminding);
  EXPECT_EQ(buffer->Size(), 40);
  ptr = buffer->Buffer() + 40;

  key = StringUtils::GenerateRandomString(10);
  value =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(64UL << 10));
  reminding =
      zone_manager_->ProcessSingleItem(buffer, key, value, [&](uint64_t lba) {
        EXPECT_EQ(lba, (10UL << 10) + 16 + 6 + 2 + (22UL << 10) + 16);
        LOG(INFO, "item flushed");
      });
  EXPECT_TRUE(reminding);
  EXPECT_EQ(buffer->Size(), 56);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
