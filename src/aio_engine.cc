#include "aio_engine.h"

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
  if (ret == -1) {
    LOG(ERROR, "aio_read failed, err: {}, offset: {}, sz: {}", strerror(errno), offset, size);
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
    int ret = aio_error(&req.aio_req_);
    if (ret == 0) {
      LOG(INFO, IORequest::GetTypeName(req.type_) +
                    ", callback offset: " + std::to_string(req.aio_req_.aio_offset));
      uint64_t finish_code = aio_return(&req.aio_req_);
      if (finish_code != 0) {
        LOG(ERROR, "return code error : {}, offset: {}, fd : {}", strerror(errno),
            req.aio_req_.aio_offset, req.aio_req_.aio_fildes);
      }
      it = requests_.erase(it);
      cnt++;
    } else {
      ++it;
    }
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