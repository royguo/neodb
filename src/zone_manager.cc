#include "zone_manager.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "codec.h"
#include "utils.h"

namespace neodb {

Status ZoneManager::Append(const std::string& key,
                           const std::shared_ptr<IOBuf>& value) {
  uint64_t idx = HashUtils::FastHash(key) % options_.writable_buffer_num_;
  // Each of the WriteBuffer can only be accessed by a single write thread.
  // TODO(Roy Guo) Wait-Free is required in the future.
  std::unique_lock<std::mutex> lk(writable_buffer_mtx_[idx]);
  auto& write_buffer = writable_buffers_[idx];
  auto status = write_buffer->Put(key, value);
  if (!status.ok()) {
    LOG(ERROR, "WriteBuffer {} put failed, key : {}, msg: {}", idx, key,
        status.msg());
    return status;
  }

  // upsert index item
  index_->Put(key, value);

  // TODO(Roy Guo) Move to background thread
  // If the WriteBuffer exceeds its capacity, we should create a new empty
  // buffer and seal the old one as immutable buffer.
  if (write_buffer->IsFull() || write_buffer->IsImmutable()) {
    write_buffer->MarkImmutable();
    {
      std::unique_lock<std::mutex> immutable_lk(immutable_buffer_mtx_);
      immutable_buffers_.emplace_back(std::move(writable_buffers_[idx]));
      writable_buffers_[idx] =
          std::make_unique<WriteBuffer>(options_.write_buffer_size_);
      LOG(DEBUG,
          "WriteBuffer {} is full, move to immutable buffer list, total "
          "immutable buffer num: {}",
          idx, immutable_buffers_.size());
    }
    immutable_buffer_cv_.notify_all();
  }
  return Status::OK();
}

// Try to pick a immutable write buffer and encode, flush it to disk.
void ZoneManager::FlushImmutableBuffers() {
  if (data_zone_ == nullptr || data_zone_->state_ == ZoneState::FULL) {
    auto s = SwitchDataZone();
    if (!s.ok()) {
      LOG(ERROR, "No available data zone for writing, retry later...");
      return;
    }
  }
  std::unique_ptr<WriteBuffer> immutable;
  {
    std::unique_lock<std::mutex> lk(immutable_buffer_mtx_);
    immutable_buffer_cv_.wait_for(lk, std::chrono::seconds(1), [&]() {
      return !immutable_buffers_.empty();
    });

    if (immutable_buffers_.empty()) {
      return;
    }

    // steal one immutable buffer out of the list
    immutable = std::move(immutable_buffers_.front());
    immutable_buffers_.pop_front();
    LOG(DEBUG,
        "Take immutable buffer for flush success, immutable buffer size: {}",
        immutable_buffers_.size());
  }

  // use the `immutable` buffer for further encoding and flushing.
  std::shared_ptr<IOBuf> encoded_buf = std::make_shared<IOBuf>(IO_FLUSH_SIZE);
  uint32_t available_sz = IO_FLUSH_SIZE;
  auto* ptr = encoded_buf->Buffer();
  auto* cur_ptr = ptr;

  std::vector<uint64_t> lba_vec;
  auto* items = immutable->GetItems();
  for (int i = 0; i < items->size(); ++i) {
    auto& item = (*items)[i];

    // Check whether current data zone can hold the next item.
    uint64_t meta_size =
        NumberUtils::AlignTo(expect_data_zone_meta_size_, IO_PAGE_SIZE);
    // IO_FLUSH_SIZE + MAX_KEY_SIZE + IO_PAGE_SIZE should be enough to hold the
    // current buffer and next item's key and related meta. So if the free space
    // is less, we should switch to another data zone.
    uint64_t max_cur_buffer_size = IO_FLUSH_SIZE;
    // key size + value size + max possible alignment size
    uint64_t max_next_item_encoded_size =
        MAX_KEY_SIZE + item.second->Size() + IO_PAGE_SIZE;
    if (data_zone_->GetAvailableBytes() <= meta_size + footer_size_ +
                                               max_cur_buffer_size +
                                               max_next_item_encoded_size) {
      // before switch to new zone, we should flush the current buffer because
      // the related LBA were already calculated.
      FlushAndResetIOBuffer(encoded_buf);
      SwitchDataZone();
      assert(data_zone_->GetUsedBytes() % IO_PAGE_SIZE == 0);
    }

    // Continue to flush next item to the new data zone.
    // Note that the flush may not flush the data into disk but only flush to
    // the IO buffer for batch processing.
    uint64_t lba = TryFlushSingleItem(encoded_buf, item.first, item.second,
                                      i == (items->size() - 1));
    lba_vec.push_back(lba);
    // Buffer the flushed key & lba and flush to the footer before zone close.
    data_zone_key_buffers_.emplace_back(item.first, lba);
  }

  // Update related index of the flushed keys
  for (uint32_t i = 0; i < items->size(); ++i) {
    auto& item = (*items)[i];
    uint64_t lba = lba_vec[i];
    index_->Update(item.first, lba);
  }
}

uint64_t ZoneManager::TryFlushSingleItem(const std::shared_ptr<IOBuf>& buf,
                                         const std::string& key,
                                         const std::shared_ptr<IOBuf>& value,
                                         bool force_flush) {
  assert(key.size() <= MAX_KEY_SIZE);
  // Expected flush LBA for current key value item.
  uint64_t lba = data_zone_->wp_ + buf->Size();

  // meta: Key Len 2B + Value Len 4B
  // TODO: add CRC to protect the meta info.
  const uint16_t meta_sz = 6;
  const uint16_t key_sz = key.size();
  const uint32_t value_sz = value->Size();
  // We should process item buffer and then reset it if there's no enough free
  // space for key data and meta
  if (buf->AvailableSize() <= (key_sz + meta_sz)) {
    LOG(DEBUG, "buffer almost full, flush it now, size: {}", buf->Size());
    // skip the last few bytes for next writing as item lba offset.
    lba += (buf->AvailableSize());
    FlushAndResetIOBuffer(buf);
    LOG(DEBUG, "buffer flushed, buffer cap: {}, buffer size: {}, lba: {}",
        buf->Capacity(), buf->Size(), data_zone_->wp_);
  }

  // Append item meta (key sz, value sz) and key data
  buf->Append(reinterpret_cast<const char*>(&key_sz), 2);
  buf->Append(reinterpret_cast<const char*>(&value_sz), 4);
  buf->Append(key.data(), key_sz);

  // Append value data, note that the value size could be much larger than the
  // buffer size.
  char* value_ptr = value->Buffer();
  char* cur_value_ptr = value_ptr;
  while (cur_value_ptr - value_ptr != value_sz) {
    uint32_t append_sz = std::min(
        buf->AvailableSize(), value_sz - uint32_t(cur_value_ptr - value_ptr));
    buf->Append(cur_value_ptr, append_sz);
    cur_value_ptr += append_sz;

    // If there's no more reminding value data, but the buffer is not yet full,
    // we should stop here, so next item can still append data to the buffer.
    if (cur_value_ptr - value_ptr == value_sz && buf->AvailableSize() > 0) {
      break;
    }
    // if there's still more value data, which means the buffer should be full
    // now, so we should process current buffer.
    // If we are lucy enough that both value data and target buffer is consumed,
    // we should reset the buffer.
    if (buf->AvailableSize() == 0) {
      LOG(DEBUG, "buffer flushed, io size: {}, lba = {}", buf->Capacity(),
          data_zone_->wp_);
      FlushAndResetIOBuffer(buf);
    }
  }

  // If the item is the last element in the WriteBuffer, then we should flush
  // the item to the disk.
  if (force_flush) {
    LOG(DEBUG, "buffer force flushed, buffer size: {}", buf->Size());
    FlushAndResetIOBuffer(buf);
  }
  return lba;
}

Status ZoneManager::ReadSingleItem(uint64_t offset, std::string* key,
                                   std::shared_ptr<IOBuf>& value) {
  const int meta_sz = 6;
  int header_read_size = IO_PAGE_SIZE;
  uint64_t align = offset % IO_PAGE_SIZE;
  if (IO_PAGE_SIZE - align < meta_sz) {
    header_read_size *= 2;
  }
  auto header = std::make_shared<IOBuf>(header_read_size);
  auto s = io_handle_->Read(offset - align, header);
  if (!s.ok()) {
    return s;
  }
  uint64_t key_sz = *reinterpret_cast<uint16_t*>(header->Buffer() + align);
  uint32_t value_sz = *reinterpret_cast<uint32_t*>(header->Buffer() + align +
                                                   2 /* skip key_sz */);
  uint64_t unaligned_total_sz = align + meta_sz + key_sz + value_sz;
  uint64_t aligned_total_sz =
      (unaligned_total_sz + IO_PAGE_SIZE - 1) / IO_PAGE_SIZE * IO_PAGE_SIZE;
  // Read out all key value data.
  value = std::make_shared<IOBuf>(aligned_total_sz);
  s = io_handle_->Read(offset - align, value);
  if (!s.ok()) {
    return s;
  }
  *key = std::string(value->Buffer() + align + meta_sz, key_sz);
  value->Shrink(align + meta_sz + key_sz, value_sz);
  return Status::OK();
}

Status ZoneManager::SwitchDataZone() {
  // Finish the current data zone before open next one.
  if (data_zone_ != nullptr) {
    LOG(INFO, "Finish the current data zone, zone id: {}", data_zone_->id_);
    auto s = FinishCurrentDataZone();
    if (!s.ok()) {
      return s;
    }
  } else {
    LOG(INFO, "No existing data zone, skip finish zone.");
  }

  // open a new empty zone.
  std::unique_lock<std::mutex> lk(empty_zones_mtx_);
  if (empty_zones_.empty()) {
    return Status::Busy();
  }
  data_zone_ = empty_zones_.back();
  empty_zones_.pop_back();
  data_zone_->state_ = ZoneState::OPEN;
  LOG(INFO, "Opened a new data zone: {}", data_zone_->id_);
  return Status::OK();
}

void ZoneManager::RecoverZoneStates() {
  // TODO (Roy Guo) Read all zone headers & footers to restore all zone status,
  // including all write pointers.
  std::unique_lock<std::mutex> lk(empty_zones_mtx_);
  for (auto& zone : zones_) {
    // TODO read zone footer to recover index

    // TODO reset unfinished zone to EMPTY state.
    
    // Push empty zones for further use.
    if (zone->state_ == ZoneState::EMPTY) {
      empty_zones_.push_back(zone);
      LOG(INFO, "Restore empty zone, id : {}, offset: {}", zone->id_,
          zone->offset_);
    }
  }
}

Status ZoneManager::FlushAndResetIOBuffer(
    const std::shared_ptr<IOBuf>& buffer) {
  buffer->AlignBufferSize();
  auto s = io_handle_->Append(data_zone_, buffer);
  if (!s.ok()) {
    LOG(ERROR, "Flush IO Buffer failed!");
    return s;
  }
  buffer->Reset();
  return Status::OK();
}

Status ZoneManager::FinishCurrentDataZone() {
  if (data_zone_ == nullptr) {
    return Status::IOError("No available data zone, finish zone skipped.");
  }

  uint64_t meta_offset = data_zone_->wp_;
  std::shared_ptr<IOBuf> zone_meta;
  Codec::GenerateDataZoneMeta(data_zone_key_buffers_, zone_meta);

  assert(data_zone_->wp_ % IO_PAGE_SIZE == 0);
  assert(zone_meta->Size() > 0);
  auto s = io_handle_->Append(data_zone_, zone_meta);
  if (!s.ok()) {
    LOG(ERROR, "finish zone failed during flush the zone meta: {}",
        std::strerror(errno));
    return s;
  }

  // This buffer includes pre-footer padding.
  auto footer = std::make_shared<IOBuf>(data_zone_->GetAvailableBytes());
  GenerateDataZoneFooter(footer, meta_offset);
  s = io_handle_->Append(data_zone_, footer);
  if (!s.ok()) {
    LOG(ERROR, "finish zone failed during flush the zone footer: {}",
        std::strerror(errno));
    return s;
  }

  // update zone state
  data_zone_->state_ = ZoneState::FULL;
  LOG(INFO, "zone finished success, id : {}", data_zone_->id_);
  return Status::OK();
}

void ZoneManager::WriteZoneHeader(const std::shared_ptr<Zone>& zone) {
  // TODO implement
  zone->state_ = ZoneState::OPEN;
  LOG(INFO, "Active new zone, id: {}, offset: {}", zone->id_, zone->offset_);
}

// Footer:
// [...]
// [key count 4B]
// [meta offset 8B] <--- zone end
void ZoneManager::GenerateDataZoneFooter(const std::shared_ptr<IOBuf>& buf,
                                         uint64_t meta_offset) {
  assert(buf->Capacity() % IO_PAGE_SIZE == 0);
  assert(buf->Capacity() >= IO_PAGE_SIZE);
  uint32_t key_cnt = data_zone_key_buffers_.size();
  buf->AppendZeros(buf->Capacity() - 12);
  buf->Append((char*)&key_cnt, 4);
  buf->Append((char*)&meta_offset, 8);
}

}  // namespace neodb