#include "neodb/histogram.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "io_handle.h"
#include "utils.h"

namespace neodb {
class HistogramTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(HistogramTest, SimpleTest) {
  std::string name = "simple_tp";
  TRACE_POINT(name, {
    LOG(INFO, "code line1 of the test trace point 1.");
    LOG(INFO, "code line2 of the test trace point 1.");
  });
  PRINT_TRACE_POINT(name);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
