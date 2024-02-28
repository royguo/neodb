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

  bool reminding = zone_manager_->FlushSingleItem(
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
  reminding = zone_manager_->FlushSingleItem(
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
      zone_manager_->FlushSingleItem(buffer, key, value, [&](uint64_t lba) {
        LOG(INFO, "item appended");
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
      zone_manager_->FlushSingleItem(buffer, key, value, [&](uint64_t lba) {
        EXPECT_EQ(lba, (10UL << 10) + 16 + 6 + 2 + (22UL << 10) + 16);
        LOG(INFO, "item appended");
      });
  EXPECT_TRUE(reminding);
  EXPECT_EQ(buffer->Size(), 56);
}

TEST_F(ZoneManagerTest, WriteAndRestTest) {
  std::shared_ptr<IOBuf> buffer = std::make_shared<IOBuf>(32UL << 10);

  // Write two items
  std::string key1 = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value1 =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(10UL << 10));
  bool reminding =
      zone_manager_->FlushSingleItem(buffer, key1, value1, [](uint64_t lba) {
        EXPECT_EQ(lba, 0);
        LOG(INFO, "item appended, lba offset: {}", lba);
      });
  EXPECT_TRUE(reminding);

  std::string key2 = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value2 =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(22496));
  reminding =
      zone_manager_->FlushSingleItem(buffer, key2, value2, [](uint64_t lba) {
        EXPECT_EQ(lba, (10UL << 10) + 16);
        LOG(INFO, "item appended, lba offset: {}", lba);
      });
  EXPECT_TRUE(reminding);

  // Read them out and verify
  std::shared_ptr<IOBuf> read_value1;
  std::string read_key1;
  zone_manager_->ReadSingleItem(0, &read_key1, read_value1);
  EXPECT_EQ(read_key1, key1);
  EXPECT_EQ(read_value1->Data(), value1->Data());

  std::shared_ptr<IOBuf> read_value2;
  std::string read_key2;
  zone_manager_->ReadSingleItem((10UL << 10) + 16, &read_key2, read_value2);
  EXPECT_EQ(read_key2, key2);
  EXPECT_EQ(read_value2->Data(), value2->Data());
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
