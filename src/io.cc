#include "io.h"

#include "logger.h"
#include "utils.h"

namespace neodb {
Status FileIOHandle::Write(uint64_t offset, std::shared_ptr<IOBuf> data) {
  uint64_t ret = pwrite(write_fd_, data->Buffer(), data->Size(), int64_t(offset));
  if (ret != data->Size()) {
    LOG(ERROR, "Failed to write data: {} ", std::strerror(errno));
    return Status::IOError("Write failed!");
  }
  return Status::OK();
}

Status FileIOHandle::ReadAppend(uint64_t offset, uint32_t size, std::shared_ptr<IOBuf> data) {
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
  assert(data->Capacity() % IO_PAGE_SIZE == 0);
  uint64_t ret = pread(read_fd_, data->Buffer(), data->Capacity(), int64_t(offset));
  if (ret != data->Capacity()) {
    LOG(ERROR, "Failed to read data, msg: {}, offset: {}, read_sz: {}, ret_sz: {}",
        std::strerror(errno), offset, data->Capacity(), ret);
    return Status::IOError(std::strerror(errno));
  }
  data->IncreaseSize(ret);
  return Status::OK();
}

Status FileIOHandle::Append(const std::shared_ptr<Zone>& zone, std::shared_ptr<IOBuf> data) {
  auto s = Write(zone->wp_, data);
  zone->wp_ += data->Size();
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
    zone->wp_ = zone_offset;
    zones.push_back(std::move(zone));
    zone_offset += zone_capacity_;
  }
  return std::move(zones);
}

void FileIOHandle::ResetZone(const std::shared_ptr<Zone>& zone) {
  zone->state_ = ZoneState::EMPTY;
  zone->wp_ = zone->offset_;
  Trim(write_fd_, zone->wp_, zone->capacity_bytes_);
}

void FileIOHandle::Trim(int fd, uint64_t offset, uint64_t sz) {
#ifndef __APPLE__
  if (fstrim(fd, offset, sz) == -1) {
    LOG(ERROR, "fstrim failed, offset: {}, length: {}, error: {}", offset, sz,
        std::strerror(errno));
    return;
  }
#else
  // TODO
#endif
}

}  // namespace neodb