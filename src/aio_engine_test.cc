#include "aio_engine.h"

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
  std::unique_ptr<AIOEngine> io_engine_;

  void SetUp() override {
    InitLogger();
    test_file_ = CreateRandomFile(file_size_);
#ifdef __APPLE__
    LOG(INFO, "Init Mocking AIO Engine");
    io_engine_ = std::make_unique<MockAIOEngine>();
#else
    LOG(INFO, "Init Posix AIO Engine");
    io_engine_ = std::make_unique<PosixAIOEngine>();
#endif
    write_fd_ = FileUtils::OpenDirectFile(test_file_);
    if (write_fd_ == -1) {
      LOG(ERROR, "Open writable file failed, error: {}", std::strerror(errno));
      abort();
    }
    read_fd_ = FileUtils::OpenDirectFile(test_file_, true);
    if (read_fd_ == -1) {
      LOG(ERROR, "Open readonly file failed, error: {}", std::strerror(errno));
      abort();
    }
    LOG(INFO, "write fd: {}, read fd: {}", write_fd_, read_fd_);
  }

  void TearDown() override {
    int total = FileUtils::DeleteFilesByPrefix(".", file_prefix);
    LOG(INFO, "{} files was deleted", total);
  }

  static void SyncRead(int fd, char* buf, uint32_t offset, uint32_t size) {
    uint64_t ret = pread(fd, (void*)buf, size, offset);
    ASSERT_TRUE(ret > 0);
  }

  std::string CreateRandomFile(uint32_t sz) {
    auto filename = FileUtils::GenerateRandomFile(file_prefix, sz);
    filenames_.emplace_back(filename);
    return filename;
  }

 private:
  std::vector<std::string> filenames_;
};

TEST_F(IOEngineTest, MultipleAIOReadWriteTest) {
  uint64_t page_size = 4UL << 10;
  uint32_t total_requests = 6;

  // Write
  char* buffers[total_requests];
  for (int i = 0; i < total_requests; ++i) {
    posix_memalign((void**)&buffers[i], page_size, page_size);
    memset(buffers[i], 0, page_size);
    std::string data = std::to_string('a' + i);
    memcpy(buffers[i], data.c_str(), data.size());
    auto s = io_engine_->AsyncWrite(write_fd_, i * page_size, buffers[i], page_size, nullptr);
    EXPECT_TRUE(s.ok());
  }

  // Poll
  uint32_t cnt = 0;
  while (cnt < total_requests) {
    cnt += io_engine_->Poll();
  }
  EXPECT_EQ(cnt, total_requests);
  EXPECT_EQ(io_engine_->GetInFlightRequests(), 0);

  // Sync Read
  char* buf = nullptr;
  posix_memalign((void**)&buf, page_size, page_size);
  for (int i = 0; i < total_requests; ++i) {
    memset(buf, 0, page_size);
    SyncRead(read_fd_, buf, i * page_size, page_size);
    std::string data = std::to_string('a' + i);
    EXPECT_EQ(data, std::string(buf, data.size()));
  }
  free(buf);

  // Async Read & Poll
  for (int i = 0; i < total_requests; ++i) {
    memset(buffers[i], 0, page_size);
    auto s = io_engine_->AsyncRead(read_fd_, i * page_size, buffers[i], page_size, nullptr);
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
    std::string data = std::to_string('a' + i);
    EXPECT_EQ(data, std::string(buffers[i], data.size()));
    free(buffers[i]);
  }
}
}  // namespace neodb
