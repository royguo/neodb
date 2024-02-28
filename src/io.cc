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
  return Status::OK();
}

Status FileIOHandle::Append(std::shared_ptr<IOBuf> data) {
  auto s = Write(wp_, data);
  wp_ += data->Size();
  return s;
}
}  // namespace neodb