#include "device.h"

#include <gtest/gtest.h>

#include <memory>

namespace neodb {
class DeviceTest : public ::testing::Test {
 public:
  std::string devname = "/tmp/neodb_mock_";

  void SetUp() override {
    // Fallocate a new file before the testing
    thread_local unsigned int seed = time(NULL);
    devname += std::to_string(rand_r(&seed) % 10000);
    int fd =
        open(devname.data(), O_RDWR | O_DIRECT | O_CREAT, S_IRUSR | S_IWUSR);
    ASSERT_GT(fd, 0);
    int ret = fallocate(fd, 0, 0, 1UL << 30 /* 1GB */);
    ASSERT_EQ(ret, 0);
    ret = close(fd);
    ASSERT_EQ(ret, 0);
  }

  void TearDown() override {
    // Remove mock file
    int r = std::remove(devname.data());
    ASSERT_EQ(r, 0);
  }
};

TEST_F(DeviceTest, SimpleTest) {
  std::unique_ptr<Device> dev;
  auto s = OpenDevice(devname, &dev);
  ASSERT_TRUE(s.ok());
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
