#include "codec.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "utils.h"

namespace neodb {
class CodecTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CodecTest, GenerateDataZoneMetaTest) {
  Codec::DataZoneKeyBuffer key_buffer;
  key_buffer.emplace_back("a", 1);
  key_buffer.emplace_back("b", 2);
  key_buffer.emplace_back("c", 3);
  std::shared_ptr<IOBuf> meta;
  uint64_t size = Codec::GenerateDataZoneMeta(key_buffer, meta);
  EXPECT_EQ(meta->Capacity(), 4UL << 10);
  EXPECT_EQ(size, 33);

  std::string large_key = StringUtils::GenerateRandomString(4UL << 10);
  key_buffer.emplace_back(large_key, 4);
  size = Codec::GenerateDataZoneMeta(key_buffer, meta);
  EXPECT_EQ(size, (4UL << 10) + 33 + 10);
  EXPECT_EQ(meta->Capacity(), 8UL << 10);

  std::vector<std::pair<std::string, uint64_t>> pairs;
  Codec::DecodeDataZoneMeta(meta->Buffer(), meta->Size(),
                            [&](const std::string& key, uint64_t lba) {
                              pairs.emplace_back(key, lba);
                            });
  EXPECT_EQ(pairs[0].first, "a");
  EXPECT_EQ(pairs[0].second, 1);
  EXPECT_EQ(pairs[1].first, "b");
  EXPECT_EQ(pairs[1].second, 2);
  EXPECT_EQ(pairs[2].first, "c");
  EXPECT_EQ(pairs[2].second, 3);
  EXPECT_EQ(pairs[3].first, large_key);
  EXPECT_EQ(pairs[3].second, 4);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace neodb