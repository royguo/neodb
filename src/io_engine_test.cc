#include "io_engine.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "utils.h"

namespace neodb {
class IOEngineTest : public ::testing::Test {
 public:
  std::string file_prefix = "io_engine_test_file_";
  std::string test_file_;
  int write_fd_{};
  int read_fd_{};
  uint64_t file_size_ = 10 << 20;
  std::unique_ptr<IOEngine> io_engine_;

  void SetUp() override {
    test_file_ = CreateRandomFile(file_size_);
    io_engine_ = std::make_unique<PosixAIOEngine>();
    write_fd_ = open(test_file_.c_str(), O_RDWR | O_CREAT, 0777);
    if (write_fd_ == -1) {
      LOG(ERROR, "Open writable file failed, error: {}", std::strerror(errno));
      abort();
    }
    read_fd_ = open(test_file_.c_str(), O_RDONLY);
    if (read_fd_ == -1) {
      LOG(ERROR, "Open readonly file failed, error: {}", std::strerror(errno));
      abort();
    }
    LOG(INFO, "write fd: {}, read fd: {}", write_fd_, read_fd_);

    //    const char* buf = "abcd";
    //    size_t ret = pwrite(write_fd_, buf, 4, 0);
    //    ASSERT_EQ(ret, 4);
    //    char* buf2 = static_cast<char*>(malloc(4));
    //    ret = pread(read_fd_, buf2, 4, 0);
    //    ASSERT_STREQ(buf2, "abcd");
  }

  void TearDown() override {
    //    int total = FileUtils::DeleteFilesByPrefix(".", file_prefix);
    //    LOG(INFO, "{} files was deleted", total);
  }

  std::string CreateRandomFile(uint32_t sz) {
    auto filename = FileUtils::GenerateRandomFile(file_prefix, sz);
    filenames_.emplace_back(filename);
    return filename;
  }

 private:
  std::vector<std::string> filenames_;
};

TEST_F(IOEngineTest, MultipleAIOWriteTest) {
  uint64_t page_size = 4UL << 10;
  uint32_t total_requests = 6;

  // Write Request
  for (int i = 0; i < total_requests; ++i) {
    std::string data = std::to_string(i);
    auto s = io_engine_->AsyncWrite(write_fd_, i * page_size, data.c_str(), data.size(), nullptr);
    EXPECT_TRUE(s.ok());
  }

  // POLL result out
  uint32_t cnt = 0;
  while (cnt < total_requests) {
    cnt += io_engine_->Poll();
  }
  EXPECT_EQ(cnt, total_requests);
  EXPECT_EQ(io_engine_->GetInFlightRequests(), 0);

  // Read Request
  char* buf = nullptr;
  posix_memalign((void**)&buf, page_size, total_requests * page_size);
  for (int i = 0; i < total_requests; ++i) {
    auto s = io_engine_->AsyncRead(read_fd_, i * page_size, buf, page_size, nullptr);
    ASSERT_TRUE(s.ok());
  }
  EXPECT_EQ(io_engine_->GetInFlightRequests(), total_requests);

  // POLL result out
  cnt = 0;
  while (cnt < total_requests) {
    cnt += io_engine_->Poll();
  }

  EXPECT_EQ(cnt, total_requests);
  for (int i = 0; i < total_requests; ++i) {
    ASSERT_EQ(buf[i], static_cast<char>(i));
  }
  delete buf;
}
}  // namespace neodb