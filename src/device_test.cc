#include "device.h"

#include <gtest/gtest.h>

#include <memory>

namespace neodb {
class DeviceTest : public ::testing::Test {
 public:
  std::string path;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(DeviceTest, SimpleTest) {
  std::unique_ptr<Device> dev = OpenDevice("/tmp/neodb_fs");
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
