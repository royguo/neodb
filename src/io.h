#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <utility>

#include "neodb/io_buf.h"
#include "neodb/logger.h"
#include "neodb/status.h"

namespace neodb {
class IOHandle {
 public:
  virtual ~IOHandle() = default;

  virtual Status Write(uint64_t offset, std::shared_ptr<IOBuf> data) = 0;

  virtual Status Read(uint64_t offset, uint32_t size,
                      std::shared_ptr<IOBuf> value) = 0;

  virtual Status Append(std::shared_ptr<IOBuf> data) = 0;

  void Seek(uint64_t wp) { wp_ = wp; }

  uint64_t GetWritePointer() { return wp_; }

 protected:
  // Append write pointer
  uint64_t wp_;
};

// class S3IOHandle : public IOHandle {};
//
// class BdevIOHandle : public IOHandle {};

class FileIOHandle : public IOHandle {
 public:
  explicit FileIOHandle(std::string filename) : filename_(std::move(filename)) {
#ifdef __APPLE__
    write_fd_ =
        open(filename_.c_str(), O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
    fcntl(write_fd_, F_NOCACHE, 1);
#else
    write_fd_ =
        open(filename_.c_str(), O_RDWR | O_DIRECT | O_CREAT, S_IRUSR | S_IWUSR);
#endif
    if (write_fd_ == -1) {
      LOG(ERROR, "open writable file failed: {}", std::strerror(errno));
      close(write_fd_);
      abort();
    }
    read_fd_ = open(filename_.c_str(), O_RDONLY | O_SYNC);
    if (read_fd_ == -1) {
      LOG(ERROR, "open readable file failed: {}", std::strerror(errno));
      close(read_fd_);
      abort();
    }
  }

  ~FileIOHandle() override {
    close(write_fd_);
    close(read_fd_);
  }

  Status Write(uint64_t offset, std::shared_ptr<IOBuf> data) override;

  Status Read(uint64_t offset, uint32_t size,
              std::shared_ptr<IOBuf> value) override;

  Status Append(std::shared_ptr<IOBuf> data) override;

 private:
  int write_fd_;
  int read_fd_;
  std::string filename_;
};
}  // namespace neodb
