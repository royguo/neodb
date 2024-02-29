#include "zone_manager.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "codec.h"
#include "utils.h"

namespace neodb {

Status ZoneManager::Append(const std::string& key,
                           const std::shared_ptr<IOBuf>& value) {
  uint64_t idx = HashUtils::FastHash(key) % options_.writable_buffer_num;
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
  // If the WriteBuffer exceeds its capacity, we should create a new empty
  // buffer and seal the old one as immutable buffer.
  if (write_buffer->IsFull() || write_buffer->IsImmutable()) {
    write_buffer->MarkImmutable();
    {
      std::unique_lock<std::mutex> immutable_lk(immutable_buffer_mtx_);
      immutable_buffers_.emplace_back(std::move(writable_buffers_[idx]));
      writable_buffers_[idx] =
          std::make_unique<WriteBuffer>(options_.write_buffer_size);
      LOG(INFO, "WriteBuffer {} is full, move to immutable buffer list", idx);
    }
  }
  return Status::OK();
}

// Try to pick a immutable write buffer and encode, flush it to disk.
void ZoneManager::TryFlush() {
  std::unique_ptr<WriteBuffer> immutable;
  {
    std::unique_lock<std::mutex> lk(immutable_buffer_mtx_);
    immutable_buffer_cv_.wait(lk,
                              [&]() { return !immutable_buffers_.empty(); });

    // steal one immutable buffer out of the list
    immutable = std::move(immutable_buffers_.front());
    immutable_buffers_.pop_front();
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
    uint64_t lba = TryFlushSingleItem(encoded_buf, item.first, item.second,
                                      i == (items->size() - 1));
    lba_vec.push_back(lba);
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
  uint64_t lba = io_handle_->GetWritePointer() + buf->Size();

  // meta: Key Len 2B + Value Len 4B
  // TODO: add CRC to protect the meta info.
  const uint16_t meta_sz = 6;
  const uint16_t key_sz = key.size();
  const uint32_t value_sz = value->Size();
  // We should process item buffer and then reset it if there's no enough free
  // space for key data and meta
  if (buf->AvailableSize() <= (key_sz + meta_sz)) {
    LOG(DEBUG, "buffer almost full, size: {}", buf->Size());
    // skip the last few bytes for next writing as item lba offset.
    lba += (buf->AvailableSize());
    buf->Reset();
    io_handle_->Append(buf);
    LOG(DEBUG, "buffer flushed, buffer cap: {}, buffer size: {}, lba: {}",
        buf->Capacity(), buf->Size(), io_handle_->GetWritePointer());
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
          io_handle_->GetWritePointer());
      io_handle_->Append(buf);
      buf->Reset();
    }
  }

  // If the item is the last element in the WriteBuffer, then we should flush
  // the item to the disk.
  if (force_flush) {
    buf->AlignBufferSize();
    io_handle_->Append(buf);
    buf->Reset();
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
  uint64_t value_sz = *reinterpret_cast<uint16_t*>(header->Buffer() + align +
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

}  // namespace neodb