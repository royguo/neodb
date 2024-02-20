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

  void SetUp() override {
    filename_ =
        FileUtils::GenerateRandomFile("zone_manager_test_file_", 1UL << 30);
    auto io_handle = std::make_unique<FileIOHandle>(filename_);
    zone_manager_ = new ZoneManager(std::move(io_handle));
  }

  void TearDown() override {
    delete zone_manager_;
    FileUtils::DeleteFile(filename_);
  }
};

TEST_F(ZoneManagerTest, EncodeSingleItemTest) {
  char* buf = nullptr;
  int ret = posix_memalign((void**)&buf, 4UL << 10, 16UL << 10);
  EXPECT_TRUE(ret == 0);
  std::string key = "key12345";
  std::string value = "value67890";
  uint32_t encoded_size = zone_manager_->EncodeSingleItem(
      buf, key.c_str(), key.size(), value.c_str(), value.size());
  EXPECT_GT(encoded_size, 0);

  std::shared_ptr<IOBuf> decoded_key;
  std::shared_ptr<IOBuf> decoded_value;
  zone_manager_->DecodeSingleItem(buf, decoded_key, decoded_value);
  EXPECT_EQ(key, decoded_key->Data());
  EXPECT_EQ(value, decoded_value->Data());
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
