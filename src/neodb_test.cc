#include <gtest/gtest.h>

namespace neodb {
class NeoDBTest : public ::testing::Test {
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(NeoDBTest, SimpleTest) {}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
