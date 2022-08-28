#include "neodb/write_buffer.h"

#include <gtest/gtest.h>

#include <memory>

namespace neodb {
class WriteBufferTest : public ::testing::Test {
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(WriteBufferTest, SimpleTest) {
  WriteBuffer buffer(128UL << 20, 4 /*queue num*/, [](const std::string& key,
                                                      uint64_t disk_ptr) {
    std::cout << "flush key: " << key << ", disk_ptr:" << disk_ptr << std::endl;
  });

  std::shared_ptr<IOBuf> val = std::make_shared<IOBuf>("test value 1");
  buffer.PushOrWait("key 1", val);
  buffer.Flush(10 /*flush queue size*/);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
