#pragma once

#include "neodb/status.h"

#include <list>
#include <map>
#include <memory>
#include <vector>

namespace neodb {
class Zone {
 public:
  enum Type : uint8_t { kWAL = 0, kMeta, kData };

 public:
  Zone() = default;
  Zone(uint64_t offset, uint32_t capacity, uint32_t wp) {}

 private:
  uint32_t zone_capacity_ = 256UL << 20;
  // write pointer, append only
  uint32_t wp_ = 0;
  // global LBA offset of current zone
  uint64_t start_offset_ = 0;
};

struct OpenZoneDeviceOptions {
  bool open_exist_db = true;
  std::string db_path;
};

// This ZoneManager is not thread-safe so we have to make sure that each disk
// can only has a single thread submitting IOs
//
// Disk Data laayout:
//   [Meta Zone] [Meta Zone] [...] [Data Zone] ... [Meta Zone] ... [...]
//
// Zone data layout:
//   [zone header 4KB][zone data][zone footer 4KB]
//
// Zone header(4KB):
//   [CRC 4B]
//   [LENGTH 4B]
//   [DB_ID 4B]
//   [SEQ_NUMBER 4B]
//   [ZONE_TYPE 1B]
//   [...]
//   [PADDING]
// Zone footer(4KB)
//
class ZoneManager {
 public:
  ZoneManager(const OpenZoneDeviceOptions& options) : options_(options) {
    if (!options.open_exist_db) {
      InitializeDB();
      return;
    }
    Recovery();
  }

  ~ZoneManager() = default;

 public:
  Status Append(uint32_t zone_id, const char* data, uint32_t size);

  Status Read();

  // Recovery from existing database.
  // If there's no existing valid data on the disk, we will initialize a new
  // database.
  // If there's some existing valid data, we will try to recover them as much as
  // possible.
  void Recovery();

  // Initialize a new database, erase all existing data.
  void InitializeDB();

 private:
  // Scan all zone headers & footers and init the zone list vector.
  void InitializeZoneVector();

 private:
  // All zone sorted by LBA asc
  std::vector<std::shared_ptr<Zone>> zones_;

  // Track all active zones for different zone types
  std::map<Zone::Type, std::list<std::shared_ptr<Zone>>> active_zones_;

  // Maximum number of zones that could be actived for write, for ZNS SSD we can
  // get this info from device. For conventional SSD we could assume that we
  // have unlimited active zone number
  int max_active_zone_number_ = 0;

  // Current zone count that has been actived for write.
  int active_zone_count_ = 0;

  OpenZoneDeviceOptions options_;
};
}
