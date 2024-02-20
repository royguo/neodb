#include "io.h"

#include "neodb/logger.h"

namespace neodb {
Status FileIOHandle::Write(uint64_t offset, uint32_t size,
                           std::shared_ptr<IOBuf> data) {
  uint64_t ret = pwrite(write_fd_, data->Buffer(), size, int64_t(offset));
  if (ret != size) {
    LOG(ERROR, "Failed to write data: {} ", std::strerror(errno));
    return Status::IOError("Write failed!");
  }
  return Status::OK();
}

Status FileIOHandle::Read(uint64_t offset, uint32_t size,
                          std::shared_ptr<IOBuf> value) {
  uint64_t ret = pread(read_fd_, value->Buffer(), size, int64_t(offset));
  value->IncreaseSize(size);
  if (ret != size) {
    LOG(ERROR, "Failed to read data: {} ", std::strerror(errno));
    return Status::IOError("Read failed!");
  }
  return Status::OK();
}
}  // namespace neodb