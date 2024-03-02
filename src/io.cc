#include "io.h"

#include "neodb/logger.h"

namespace neodb {
Status FileIOHandle::Write(uint64_t offset, std::shared_ptr<IOBuf> data) {
  uint64_t ret =
      pwrite(write_fd_, data->Buffer(), data->Size(), int64_t(offset));
  if (ret != data->Size()) {
    LOG(ERROR, "Failed to write data: {} ", std::strerror(errno));
    return Status::IOError("Write failed!");
  }
  return Status::OK();
}

Status FileIOHandle::ReadAppend(uint64_t offset, uint32_t size,
                                std::shared_ptr<IOBuf> data) {
  assert(data->AvailableSize() >= size);
  uint64_t ret = pread(read_fd_, data->Buffer(), size, int64_t(offset));
  data->IncreaseSize(size);
  if (ret != size) {
    LOG(ERROR, "Failed to read data: {} ", std::strerror(errno));
    return Status::IOError("Read failed!");
  }
  return Status::OK();
}

Status FileIOHandle::Read(uint64_t offset, std::shared_ptr<IOBuf> data) {
  uint64_t ret =
      pread(read_fd_, data->Buffer(), data->Capacity(), int64_t(offset));
  if (ret != data->Capacity()) {
    LOG(ERROR, "Failed to read data, msg: {}", std::strerror(errno));
    return Status::IOError(std::strerror(errno));
  }
  data->IncreaseSize(ret);
  return Status::OK();
}

Status FileIOHandle::Append(std::shared_ptr<IOBuf> data) {
  auto s = Write(wp_, data);
  wp_ += data->Size();
  return s;
}

std::vector<std::shared_ptr<Zone>> FileIOHandle::GetDeviceZones() {
  std::vector<std::shared_ptr<Zone>> zones;
  assert(file_size_ > zone_capacity_);
  uint64_t zone_offset = 0;
  while (zone_offset < file_size_) {
    auto zone = std::make_shared<Zone>();
    zone->id_ = zone_offset / zone_capacity_;
    zone->offset_ = zone_offset;
    zone->capacity_bytes_ = zone_capacity_;
    zone->used_bytes_ = 0;
    zone->wp_ = zone_offset;
    zones.push_back(std::move(zone));
    zone_offset += zone_capacity_;
  }
  return std::move(zones);
}
}  // namespace neodb