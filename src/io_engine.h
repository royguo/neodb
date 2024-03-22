#pragma once
#include <aio.h>
#include <unistd.h>

#include <list>
#include <string>

#include "logger.h"
#include "neodb/status.h"

namespace neodb {

enum IORequestType { kAsyncWrite, kAsyncRead };
struct IORequest {
  struct aiocb aio_req_;
  IORequestType type_;

  static inline std::string GetTypeName(IORequestType type);
};

// An IO Engine could be either io_uring, posix aio, libaio or spdk io.
class IOEngine {
 public:
  IOEngine() = default;

  virtual ~IOEngine() = default;

  virtual Status AsyncWrite(int fd, uint64_t offset, const char* buffer, uint64_t size,
                            char** cb) = 0;

  virtual Status AsyncRead(int fd, uint64_t offset, char* buffer, uint64_t size, char** cb) = 0;

  // Poll the result and run the callback functions of each IO request.
  // @return The total number of finished IO requests.
  virtual uint32_t Poll() = 0;

  virtual uint32_t GetInFlightRequests() = 0;

 protected:
  // maximum in-flight IO requests. If exceeded, we should wait.
  int io_depth_ = 20;
};

// aio.h wrapper
class PosixAIOEngine : public IOEngine {
 public:
  PosixAIOEngine() = default;

  ~PosixAIOEngine() override = default;

  // TODO Zero-copy to DMA memory.
  Status AsyncWrite(int fd, uint64_t offset, const char* buffer, uint64_t size, char** cb) override;

  Status AsyncRead(int fd, uint64_t offset, char* buffer, uint64_t size, char** cb) override;

  // TODO Probably we should check the event by their submission order?
  uint32_t Poll() override;

  uint32_t GetInFlightRequests() override { return requests_.size(); }

 private:
  std::list<IORequest> requests_;
};

// libaio.h wrapper
// class KernelAIOEngine : public IOEngine {};

// io_uring wrapper implementation
// class IOUringEngine : public IOEngine {};

}  // namespace neodb