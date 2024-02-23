#pragma once
#include <list>
#include <memory>
#include <utility>

#include "io.h"
#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "write_buffer.h"

namespace neodb {

class ZoneManager {
 public:
  explicit ZoneManager(DBOptions options, std::unique_ptr<IOHandle> io_handle)
      : options_(std::move(options)), io_handle_(std::move(io_handle)) {
    for (int i = 0; i < options_.writable_buffer_num; ++i) {
      writable_buffers_.emplace_back(new WriteBuffer());
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

  Status Append(const std::string& key, std::shared_ptr<IOBuf> value);

  // Encode a single key value item into the target buffer. If the target buffer
  // is full, then use `func` as callback to process the buffer. Then continue
  // to encode the rest of the key value data. If the buffer is not full and the
  // key value data is fully consumed, then stop the processing. Note that after
  // the processing, the target buffer may still contain some data.
  //
  // @return True if the target buffer has un processed data.
  bool ProcessSingleItem(
      std::shared_ptr<IOBuf> buf, const std::string& key,
      std::shared_ptr<IOBuf> value,
      std::function<void(std::shared_ptr<IOBuf>, bool /*reset buffer*/)>);

  void TryFlush();

  //  uint32_t Size() { return items_.size(); }

  // Compress the target value and add CRC.
  // Format: [CRC 4B][Key Length 2B][Value Length 4B][KV with CRC]
  // [KV WITH CRC] means if the total size of the buffer exceeds block size
  // (32K) We will add new CRC in the middle of the KV item.
  //
  // @param buf Pre-allocated buffer
  // @param wp Write pointer of the buffer where we should put our encoded data.
  // @return Encoded buffer size
  //  uint32_t EncodeSingleItem(char* buf, uint32_t wp, const char* key,
  //                            uint16_t key_sz, const char* value,
  //                            uint32_t value_sz);

  // Decompress the target value and verify CRC
  // @param encoded_buf The encoded kv buffer with CRC and compression
  // @param key Decoded key
  // @param value Decoded value
  //  void DecodeSingleItem(const char* encoded_buf, std::shared_ptr<IOBuf> key,
  //                        std::shared_ptr<IOBuf> value);

 private:
  DBOptions options_;

  std::unique_ptr<IOHandle> io_handle_;

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