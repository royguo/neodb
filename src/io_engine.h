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
  // TODO change to a more generalize member variable instead of using posix aio data structure.
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

  virtual uint32_t GetInFlightRequests() { return requests_.size(); }

 protected:
  // maximum in-flight IO requests. If exceeded, we should wait.
  int io_depth_ = 20;

  std::list<IORequest> requests_;
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

 private:
  std::list<IORequest> requests_;
};

// A MockAIOEngine uses sync IO to emulate the async engine.
class MockAIOEngine : public IOEngine {
 public:
  MockAIOEngine() = default;

  ~MockAIOEngine() override = default;

  Status AsyncWrite(int fd, uint64_t offset, const char* buffer, uint64_t size, char** cb) override;

  Status AsyncRead(int fd, uint64_t offset, char* buffer, uint64_t size, char** cb) override;

  // The mock aio engine does nothing in Poll().
  uint32_t Poll() override;

 private:
};

// libaio.h wrapper
// class KernelAIOEngine : public IOEngine {};

// io_uring wrapper implementation
// class IOUringEngine : public IOEngine {};

}  // namespace neodb