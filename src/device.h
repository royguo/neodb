#pragma once
#include <fcntl.h>
#include <libzbd/zbd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <memory>

#include "neodb/status.h"

namespace neodb {
struct DeviceInfo {
  // The LBA size of each zone
  uint32_t zone_size = 256UL << 20;
  // The zone capacity (usable space) of each zone
  uint32_t zone_capacity = 256UL << 20;
  // Total zone number of current device
  uint32_t zone_count = 20;
};

class Device {
 public:
  Device() = default;
  virtual ~Device() {}

  virtual DeviceInfo GetInfo() = 0;
  // Write the data in buf onto disk
  virtual Status Write(uint64_t offset, const char* buf, uint32_t size) {
    return Status::OK();
  }
  // Read data from disk, fill them in the pre allocated buf.
  virtual Status Read(uint64_t offset, uint32_t size, char* buf) {
    return Status::OK();
  }
};

// A file-based mocking device (>= 1GB is required)
class FileDevice : public Device {
 public:
  FileDevice(const std::string& path, uint64_t size)
      : path_(path), size_(size) {
    read_fd_ = open(path.c_str(), O_DIRECT | O_RDONLY);
    write_fd_ = open(path.c_str(), O_DIRECT | O_WRONLY);
  }

  ~FileDevice() {}

  DeviceInfo GetInfo() override {
    DeviceInfo info;
    // We require a file-based device should at least larger than 1GB.
    assert(size_ >= (1UL << 30));
    info.zone_size = size_ / info.zone_count;
    info.zone_capacity = info.zone_size;
    return info;
  }

  Status Write(uint64_t offset, const char* buf, uint32_t size) {
    uint64_t ret = pwrite(write_fd_, buf, size, offset);
    // TODO(kuankuan.guo@foxmail.com) Add error handler
    assert(ret == size);
    return Status::OK();
  }

  Status Read(uint64_t offset, uint32_t size, char* buf) {
    uint64_t ret = pread(read_fd_, buf, size, offset);
    return Status::OK();
  }

 private:
  std::string path_;
  // Maximum file size, a file based device should be pre-allocated
  uint64_t size_;
  // current device's fd
  int read_fd_;
  int write_fd_;
};

// A conventional SSD or HDD block device
class ConvDevice : public Device {
 public:
  ConvDevice(const std::string& path, uint64_t size)
      : path_(path), size_(size) {}
  ~ConvDevice() {}

  DeviceInfo GetInfo() override {
    DeviceInfo info;
    return info;
  }

 private:
  std::string path_;
  uint64_t size_;
};

/*
// A Native ZNS SSD block device
class ZNSSSDDevice : public Device {};

// A SMR HDD device
class SMRHDDDevice : public Device {};
*/
// Open device based on its passed-in path
static Status OpenDevice(const std::string& path,
                         std::unique_ptr<Device>* device) {
  bool zoned = zbd_device_is_zoned(path.c_str());
  // If not a zoned device, it could be either a conventional bdev or a
  // file-based device.
  if (!zoned) {
    struct stat dev_stat;
    int err = stat(path.c_str(), &dev_stat);
    if (err < 0) {
      return Status::IOError("fstat failed!");
    }

    if (S_ISREG(dev_stat.st_mode)) {  // Regular File
      device->reset(new ConvDevice(path, dev_stat.st_size));
    } else if (S_ISBLK(dev_stat.st_mode)) {  // Block device
      device->reset(new FileDevice(path, dev_stat.st_size));
    } else {
      return Status::IOError("unknown device type");
    }
    return Status::OK();
  }

  // If its a zoned device, we should use libzbd open it
  // TODO(kuankuan.guo@foxmail.com) Add SMR or ZNS SSD device support
  zbd_info info;
  zbd_open(path.c_str(), O_WRONLY | O_DIRECT, &info);
  return Status::OK();
}

static void CloseDevice(int fd) {}
}  // namespace neodb
