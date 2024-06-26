#include "utils.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "io_handle.h"

namespace neodb {
class UtilsTest : public ::testing::Test {
 public:
  void SetUp() override { InitLogger(); }

  void TearDown() override {
    // Delete all files that was generated by this UT.
    for (auto& name : filenames_) {
      RemoveFile(name);
    }
  }

  std::string CreateRandomFile(uint32_t sz) {
    auto filename = FileUtils::GenerateRandomFile("utils_test_file_", sz);
    filenames_.emplace_back(filename);
    return filename;
  }

  static bool RemoveFile(const std::string& filename) { return FileUtils::DeleteFile(filename); }

 private:
  std::vector<std::string> filenames_;
};

TEST_F(UtilsTest, SimpleTest) {
  auto filename = CreateRandomFile(1UL << 30);
  EXPECT_TRUE(RemoveFile(filename));
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
