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

  DataPointer pointer;
  pointer.on_disk = true;
  pointer.disk_ptr = 10010ULL;
  idx.Put("key1", pointer);

  // Found
  DataPointer ret;
  auto s = idx.Get("key1", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_TRUE(ret.on_disk);
  ASSERT_TRUE(ret.disk_ptr == pointer.disk_ptr);

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
