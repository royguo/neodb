#include "neodb/index.h"

#include <gtest/gtest.h>

#include <memory>

namespace neodb {
class IndexTest : public ::testing::Test {
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(IndexTest, SimpleTest) {
  Index idx;

  DataPointer pointer{0x1001UL, false};
  idx.Put("key1", pointer);

  // Found
  DataPointer ret;
  auto s = idx.Get("key1", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_TRUE(ret.address == pointer.address);

  // Not found
  s = idx.Get("key2", &ret);
  ASSERT_FALSE(s.ok());
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
