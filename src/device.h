#pragma once
#include <memory>
#include "neodb/status.h"

#include <fcntl.h>
#include <libzbd/zbd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace neodb {
struct DeviceInfo {
  // The LBA size of each zone
  uint32_t zone_size = 256UL << 20;
  // The zone capacity (usable space) of each zone
  uint32_t zone_capacity = 256UL << 20;
  // Total zone number of current device
  uint32_t zone_count = 0;
};

class Device {
 public:
  Device() = default;
  virtual ~Device() {}

  virtual DeviceInfo GetInfo() = 0;

  /*
  virtual Status Write();

  virtual Status Read();
  */
};

// A file-based mocking device
class FileDevice : public Device {
 public:
  FileDevice(const std::string& path, uint64_t size)
      : path_(path), size_(size) {}

  ~FileDevice() {}

  DeviceInfo GetInfo() override {
    DeviceInfo info;
    return info;
  }

  /*
    Status Write() override;

    Status Read() override;
    */

 private:
  std::string path_;
  // Maximum file size, a file based device should be pre-allocated
  uint64_t size_;
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
}
