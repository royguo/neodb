#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <utility>

#include "logger.h"
#include "neodb/io_buf.h"
#include "neodb/status.h"
#include "zone.h"

namespace neodb {
class IOHandle {
 public:
  virtual ~IOHandle() = default;

  virtual Status Write(uint64_t offset, std::shared_ptr<IOBuf> data) = 0;

  // Read certain amount of data and append to the data buffer.
  // The data is pre-allocated, and its free space should be enough.
  virtual Status ReadAppend(uint64_t offset, uint32_t size,
                            std::shared_ptr<IOBuf> data) = 0;

  // Read data into the pre-allocated buffer with the size of data capacity.
  virtual Status Read(uint64_t offset, std::shared_ptr<IOBuf> data) = 0;

  virtual Status Append(std::shared_ptr<IOBuf> data) = 0;

  virtual std::vector<std::shared_ptr<Zone>> GetDeviceZones() = 0;

  virtual void Seek(uint64_t wp) { wp_ = wp; };

  virtual uint64_t GetWritePointer() { return wp_; }

 protected:
  // Append write pointer
  uint64_t wp_;
};

// class S3IOHandle : public IOHandle {};
//
// class BdevIOHandle : public IOHandle {};

class FileIOHandle : public IOHandle {
 public:
  explicit FileIOHandle(std::string filename, uint64_t file_size,
                        uint64_t zone_capacity)
      : filename_(std::move(filename)),
        file_size_(file_size),
        zone_capacity_(zone_capacity) {
    if (filename_.empty() || file_size_ == 0 || zone_capacity_ == 0) {
      LOG(ERROR, "FileIOHandle init failed, filename: {}, size: {}", filename_,
          file_size_);
      abort();
    }
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

  Status ReadAppend(uint64_t offset, uint32_t size,
                    std::shared_ptr<IOBuf> data) override;

  Status Read(uint64_t offset, std::shared_ptr<IOBuf> data) override;

  Status Append(std::shared_ptr<IOBuf> data) override;

  std::vector<std::shared_ptr<Zone>> GetDeviceZones() override;

 private:
  int write_fd_;

  int read_fd_;

  std::string filename_;

  uint64_t file_size_;

  uint64_t zone_capacity_;
};
}  // namespace neodb
