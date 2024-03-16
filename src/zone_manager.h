#pragma once
#include <functional>
#include <list>
#include <memory>
#include <numeric>
#include <queue>
#include <utility>

#include "index.h"
#include "io.h"
#include "neodb/io_buf.h"
#include "neodb/options.h"
#include "neodb/status.h"
#include "write_buffer.h"
#include "zone.h"

namespace neodb {

class ZoneManager {
 public:
  explicit ZoneManager(StoreOptions options, std::unique_ptr<IOHandle> io_handle,
                       const std::shared_ptr<Index>& index)
      : options_(std::move(options)), io_handle_(std::move(io_handle)), index_(index) {
    for (int i = 0; i < options_.writable_buffer_num_; ++i) {
      writable_buffers_.emplace_back(new WriteBuffer(options_.write_buffer_size_));
    }
    // We cannot `resize` the writable_buffer_mtx_ directly because std::mutex
    // is not movable but the std::vector is.
    std::vector<std::mutex> mutexes(options_.writable_buffer_num_);
    writable_buffer_mtx_ = std::move(mutexes);
    zones_ = io_handle_->GetDeviceZones();
    RecoverZoneStates(options_.recover_exist_db_);
    // After db initialized, we should switch to a new empty zone for writing.
    SwitchDataZone();
  }

  // Start a dedicated flush worker
  void StartFlushWorker() {
    flush_worker_ = std::thread([&]() {
      // IIF all buffers were flushed and the flag was turned off, we can stop background job.
      while (!flush_worker_stopped_ || !immutable_buffers_.empty()) {
        FlushImmutableBuffers();
      }
      // When the thread is about to stop, we should flush all immutable buffers.
      for (auto& buffer : writable_buffers_) {
        if (!buffer->GetItems()->empty()) {
          buffer->MarkImmutable();
          immutable_buffers_.emplace_back(std::move(buffer));
        }
      }
      // Flush all reminding data and finish the last zone.
      while (!immutable_buffers_.empty()) {
        FlushImmutableBuffers();
      }
      FinishCurrentDataZone();
      LOG(INFO, "ZoneManager flush worker stopped, immutable buffers {}, writable buffers: {}",
          immutable_buffers_.size(), writable_buffers_.size());
    });
    LOG(INFO, "ZoneManager flush worker started");
  }

  void StopFlushWorker() {
    flush_worker_stopped_ = true;
    flush_worker_.join();
  }

  void StartGCWorker() {
    gc_worker_ = std::thread([&]() {
      while (!gc_worker_stopped_) {
        if (empty_zones_.size() < options_.gc_threshold_zone_num_) {
          GC();
        } else {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }
      LOG(INFO, "ZoneManager gc worker stopped");
    });
    LOG(INFO, "ZoneManager gc worker started");
  }

  void StopGCWorker() {
    gc_worker_stopped_ = true;
    gc_worker_.join();
  }

  Status Append(const std::string& key, const std::shared_ptr<IOBuf>& value);

  // Obtain an immutable buffer and flush its items to the disk.
  void FlushImmutableBuffers();

  // Encode a single key value item into the target buffer. If the target buffer
  // is full,we we will flush to disk. Then continue to encode the rest of the
  // key value data. If the buffer is not full and the key value data is fully
  // consumed, then stop the processing. Note that after the processing, the
  // target buffer may still contain some data.
  //
  // @param force_flush Flush to disk even if the buffer is not yet full.
  // @return The flushed item's target LBA (possible not yet flushed to disk)
  uint64_t TryFlushSingleItem(const std::shared_ptr<IOBuf>& buf, const std::string& key,
                              const std::shared_ptr<IOBuf>& value, bool force_flush = false);

  // Flush a single IO buffer.
  // The IO buffer holds a list of encoded items and should be aligned before
  // the flushing. Note that the buffer will be reset to empty for further usage
  // after the resetting.
  Status FlushAndResetIOBuffer(const std::shared_ptr<IOBuf>& buffer);

  // Finish the zone means to flush the zone meta and zone footer, reject all
  // further write requests and mark the zone as FULL.
  Status FinishCurrentDataZone();

  // Read a single key value item from the device
  // @param offset The item's LBA offset on the device, including item meta.
  Status ReadSingleItem(uint64_t offset, std::string* key, std::shared_ptr<IOBuf>& value);

  uint32_t GetWritableBufferNum() const { return writable_buffers_.size(); }

  uint32_t GetImmutableBufferNum() const { return immutable_buffers_.size(); }

  // Get a new empty zone from the empty list and use it a the current data
  // zone.
  Status SwitchDataZone();

  // Recover all existing zones.
  void RecoverZoneStates(bool reuse_db = false);

  // Select a few zones for GC.
  void GC();

  // Read data zone's footer and then get the meta buffer.
  void ReadDataZoneMeta(const std::shared_ptr<Zone>& zone, std::shared_ptr<IOBuf>& meta_buf);

  std::shared_ptr<Zone> GetCurrentDataZone() { return data_zone_; }

 private:
  const uint64_t footer_size_ = IO_PAGE_SIZE;

  StoreOptions options_;

  std::unique_ptr<IOHandle> io_handle_;

  std::shared_ptr<Index> index_;

  // Only a single data zone is writable at the same time to fit the append-only
  // IO pattern.
  // data_zone_ only takes zone from the empty zone list.
  std::shared_ptr<Zone> data_zone_;

  // This map holds all the key->LBA items.
  // Before the data zone closes, we should generate zone meta from this map and
  // flush it. During the recovery process, we should be able to recovery all
  // the items from the meta.
  std::list<std::pair<std::string, uint64_t>> data_zone_key_buffers_;
  // Accurate size of the data zone's meta, which should be flushed before the
  // 4KB zone footer.
  uint64_t expect_data_zone_meta_size_ = 0;

  // TODO We probably need a meta zone to do some checkpoints.
  std::shared_ptr<Zone> meta_zone_;

  // We will have a background thread to check & reset old zones to empty state.
  std::vector<std::shared_ptr<Zone>> empty_zones_;
  std::mutex empty_zones_mtx_;
  std::condition_variable empty_zones_cv_;

  // All physical zones, will be inited on startup and will never change the
  // list again during the system running.
  std::vector<std::shared_ptr<Zone>> zones_;

  std::vector<std::unique_ptr<WriteBuffer>> writable_buffers_;
  // Each of the write buffer slot need to have a lock.
  std::vector<std::mutex> writable_buffer_mtx_;

  std::list<std::unique_ptr<WriteBuffer>> immutable_buffers_;
  std::condition_variable immutable_buffer_cv_;
  std::mutex immutable_buffer_mtx_;

  std::atomic<bool> flush_worker_stopped_{false};

  std::atomic<bool> gc_worker_stopped_{false};

  std::thread flush_worker_;

  std::thread gc_worker_;
};
}  // namespace neodb