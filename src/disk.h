#pragma once

#include "io.h"
#include "neodb/status.h"

namespace neodb {
enum DiskType { kFILE, kBdev, kS3 };

// A Disk represents a single disk storage engine.
class Disk {
 public:
  Disk() = default;
  ~Disk() = default;

  Status Put();

  Status Get();

  bool Exist();
};

class FileDisk : public Disk {
 public:
 private:
  std::string file_path_;
};
}  // namespace neodb