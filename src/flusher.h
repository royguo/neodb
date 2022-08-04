#pragma once
#include <memory>
#include "neodb/write_buffer.h"

namespace neodb {
// A flush worker flushes a set of write buffer
class FlushWorker {
 public:
  FlushWorker() {}

 public:
  void PushBuffer(std::shared_ptr<WriteBuffer> buffer) {
    buffers_.push_back(buffer);
  }

  // Start a working thread and call DoFlush once there is a record exist.
  void Start(std::function<void(const DataPointer& pointer)>& callback);

  // Pop out a record and flush it into disk
  void DoFlush();

 private:
  std::vector<std::shared_ptr<WriteBuffer>> buffers_;
};
}
