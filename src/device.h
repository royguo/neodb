#pragma once
#include <memory>
#include "neodb/status.h"

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

  /*
    virtual DeviceInfo GetInfo();

    virtual Status Write();

    virtual Status Read();
    */
};

// A file-based mocking device
class FileDevice : public Device {
 public:
  FileDevice(const std::string& path) : path_(path) {}

  ~FileDevice() {}

  /*
    DeviceInfo GetInfo() override;

    Status Write() override;

    Status Read() override;
    */

 private:
  std::string path_;
};

/*
// A conventional SSD block device
class ConvSSDDevice : public Device {};

// A Native ZNS SSD block device
class ZNSSSDDevice : public Device {};

// A SMR HDD device
class SMRHDDDevice : public Device {};
*/
// Open device based on its passed-in path
static std::unique_ptr<Device> OpenDevice(const std::string& path) {
  // TODO
  return std::make_unique<FileDevice>(path);
}
}
