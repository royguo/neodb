#include "aio_engine.h"

#include <aio.h>

namespace neodb {
inline std::string IORequest::GetTypeName(IORequestType type) {
  switch (type) {
    case kAsyncWrite:
      return "kAsyncWrite";
    case kAsyncRead:
      return "kAsyncRead";
  }
  return "NotKnown";
}

Status PosixAIOEngine::AsyncWrite(int fd, uint64_t offset, const char* buffer, uint64_t size,
                                  char** cb) {
  if (requests_.size() >= io_depth_) {
    LOG(ERROR, "io depth full, please try again later, cur: {}", io_depth_);
    return Status::Busy();
  }

  requests_.push_back({.aio_req_ = {}, .type_ = kAsyncWrite});
  auto& request = requests_.back();
  memset(&request.aio_req_, 0, sizeof(struct aiocb));
  request.aio_req_.aio_offset = (off_t)offset;
  request.aio_req_.aio_buf = (void*)buffer;
  request.aio_req_.aio_nbytes = size;
  request.aio_req_.aio_fildes = fd;

  int ret = aio_write(&request.aio_req_);
  if (ret == -1) {
    auto msg = "aio_write failed, err msg: " + std::string(strerror(errno));
    LOG(ERROR, msg);
    requests_.pop_back();
    return Status::IOError(msg);
  }

  return Status::OK();
}

Status PosixAIOEngine::AsyncRead(int fd, uint64_t offset, char* buffer, uint64_t size, char** cb) {
  if (requests_.size() >= io_depth_) {
    LOG(ERROR, "io depth full, please try again later, cur: {}", io_depth_);
    return Status::Busy();
  }
  requests_.push_back({.aio_req_ = {}, .type_ = kAsyncRead});
  auto& request = requests_.back();
  memset(&request.aio_req_, 0, sizeof(struct aiocb));
  request.aio_req_.aio_offset = (off_t)offset;
  request.aio_req_.aio_buf = (void*)buffer;
  request.aio_req_.aio_nbytes = size;
  request.aio_req_.aio_fildes = fd;

  int ret = aio_read(&request.aio_req_);
  // If request submission failed
  if (ret == -1) {
    LOG(ERROR, "aio_read failed, ret: {}, err: {}, offset: {}, sz: {}", ret, strerror(errno),
        offset, size);
    requests_.pop_back();
    return Status::IOError("aio read failed!");
  }
  return Status::OK();
}

// TODO Probably we should check the event by their submission order?
uint32_t PosixAIOEngine::Poll() {
  uint32_t cnt = 0;
  auto it = requests_.begin();
  while (it != requests_.end()) {
    auto& req = *it;
    int err_value = aio_error(&req.aio_req_);

    // If the request was canceld, we remove & skip it.
    if (err_value == ECANCELED) {
      it = requests_.erase(it);
      continue;
    }

    // If the request is not yet completed, we skip it.
    if (err_value == EINPROGRESS) {
      ++it;
      continue;
    }

    // If the request has an error.
    if (err_value != 0) {
      LOG(ERROR, "I/O failed, aio_error: {}, errmsg: {}, offset: {}", err_value, strerror(errno),
          req.aio_req_.aio_offset);
      it = requests_.erase(it);
      continue;
    }

    // If there's no operation error occurs, we should check the return value.
    int ret_value = aio_return(&req.aio_req_);
    if (ret_value == -1) {
      LOG(ERROR, "I/O failed, aio_return: {}, errmsg: {}, offset: {}", ret_value, strerror(errno),
          req.aio_req_.aio_offset);
      it = requests_.erase(it);
      continue;
    }

    // Otherwise, the operation success
    it = requests_.erase(it);
    cnt++;
  }
  return cnt;
}

// This is a mocking async write which implemented by sync write.
Status MockAIOEngine::AsyncWrite(int fd, uint64_t offset, const char* buffer, uint64_t size,
                                 char** cb) {
  if (requests_.size() >= io_depth_) {
    LOG(ERROR, "io depth full, please try again later, cur: {}", io_depth_);
    return Status::Busy();
  }
  requests_.push_back({.aio_req_ = {}, .type_ = kAsyncWrite});
  auto ret = pwrite(fd, buffer, size, offset);
  if (ret != size) {
    LOG(ERROR, "pwrite error : {}, offset: {}, fd : {}", strerror(errno), offset, fd);
    return Status::IOError();
  }
  return Status::OK();
}

Status MockAIOEngine::AsyncRead(int fd, uint64_t offset, char* buffer, uint64_t size, char** cb) {
  if (requests_.size() >= io_depth_) {
    LOG(ERROR, "io depth full, please try again later, cur: {}", io_depth_);
    return Status::Busy();
  }
  requests_.push_back({.aio_req_ = {}, .type_ = kAsyncRead});
  auto ret = pread(fd, buffer, size, offset);
  if (ret != size) {
    LOG(ERROR, "pread error : {}, offset: {}, fd : {}", strerror(errno), offset, fd);
    return Status::IOError();
  }
  return Status::OK();
}

uint32_t MockAIOEngine::Poll() {
  uint32_t cnt = requests_.size();
  requests_.clear();
  return cnt;
}

}  // namespace neodb
