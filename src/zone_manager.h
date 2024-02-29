#pragma once
#include <list>
#include <memory>
#include <utility>

#include "index.h"
#include "io.h"
#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "write_buffer.h"

namespace neodb {

class ZoneManager {
 public:
  explicit ZoneManager(DBOptions options, std::unique_ptr<IOHandle> io_handle,
                       const std::shared_ptr<Index>& index)
      : options_(std::move(options)),
        io_handle_(std::move(io_handle)),
        index_(index) {
    for (int i = 0; i < options_.writable_buffer_num; ++i) {
      writable_buffers_.emplace_back(
          new WriteBuffer(options_.write_buffer_size));
    }
    // We cannot `resize` the writable_buffer_mtx_ directly because std::mutex
    // is not movable but the std::vector is.
    std::vector<std::mutex> mutexes(options_.writable_buffer_num);
    writable_buffer_mtx_ = std::move(mutexes);
  }

  // Start a dedicated flush worker
  void StartFlushWorker() {
    flush_worker_started_ = true;
    flush_worker_ = std::thread([&]() {
      while (flush_worker_started_) {
        TryFlush();
      }
      LOG(INFO, "ZoneManager FlushWorker stopped");
    });
    LOG(INFO, "ZoneManager FlushWorker started");
  }

  void StopFlushWorker() { flush_worker_started_ = false; }

  Status Append(const std::string& key, const std::shared_ptr<IOBuf>& value);

  // Encode a single key value item into the target buffer. If the target buffer
  // is full, then use `func` as callback to process the buffer. Then continue
  // to encode the rest of the key value data. If the buffer is not full and the
  // key value data is fully consumed, then stop the processing. Note that after
  // the processing, the target buffer may still contain some data.
  //
  // @param force_flush Flush to disk even if the buffer is not yet full.
  // @return The flushed item's target LBA (possible not yet flushed to disk)
  uint64_t TryFlushSingleItem(const std::shared_ptr<IOBuf>& buf,
                              const std::string& key,
                              const std::shared_ptr<IOBuf>& value,
                              bool force_flush = false);

  // Read a single key value item from the device
  // @param offset The item's LBA offset on the device, including item meta.
  Status ReadSingleItem(uint64_t offset, std::string* key,
                        std::shared_ptr<IOBuf>& value);

  void TryFlush();

  uint32_t GetWritableBufferNum() const { return writable_buffers_.size(); }

  uint32_t GetImmutableBufferNum() const { return immutable_buffers_.size(); }

 private:
  DBOptions options_;

  std::unique_ptr<IOHandle> io_handle_;

  std::shared_ptr<Index> index_;

  std::vector<std::unique_ptr<WriteBuffer>> writable_buffers_;
  // Each of the write buffer slot need to have a lock.
  std::vector<std::mutex> writable_buffer_mtx_;

  std::list<std::unique_ptr<WriteBuffer>> immutable_buffers_;
  std::condition_variable immutable_buffer_cv_;
  std::mutex immutable_buffer_mtx_;

  std::atomic<bool> flush_worker_started_{false};

  std::thread flush_worker_;
};
}  // namespace neodb