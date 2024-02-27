#include "zone_manager.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "codec.h"
#include "utils.h"

namespace neodb {

#define FLUSH_IO_SIZE (32UL << 10)
// TODO(Roy Guo) Shall we limit the key size?
#define MAX_KEY_SIZE (1024)

Status ZoneManager::Append(const std::string& key,
                           std::shared_ptr<IOBuf> value) {
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
  if (write_buffer->IsFull() && !write_buffer->IsImmutable()) {
    write_buffer->MarkImmutable();
    {
      std::unique_lock<std::mutex> immutable_lk(immutable_buffer_mtx_);
      immutable_buffers_.emplace_back(std::move(writable_buffers_[idx]));
      writable_buffers_[idx] = std::make_unique<WriteBuffer>();
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
  std::shared_ptr<IOBuf> encoded_buf = std::make_shared<IOBuf>(FLUSH_IO_SIZE);
  uint32_t available_sz = FLUSH_IO_SIZE;
  auto* ptr = encoded_buf->Buffer();
  auto* cur_ptr = ptr;

  std::vector<uint64_t> lba_vec;
  auto* items = immutable->GetItems();
  for (auto& item : *items) {
    // TODO Get current lba write pointer
    // uint64_t current_lba = io_handle_->
    ProcessSingleItem(encoded_buf, item.first, item.second,
                      [&lba_vec](uint64_t lba) { lba_vec.push_back(lba); });
  }
  // Flush the reminding bytes of the buffer.
  if (encoded_buf->AvailableSize() < FLUSH_IO_SIZE) {
    io_handle_->Append(encoded_buf);
  }

  // TODO(Roy Guo) Update related index of the keys

  // TODO(Roy Guo) Release the immutable buffer and free memory
}

bool ZoneManager::ProcessSingleItem(const std::shared_ptr<IOBuf>& buf,
                                    const std::string& key,
                                    const std::shared_ptr<IOBuf>& value,
                                    const std::function<void(uint64_t)>& func) {
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
  func(lba);
  return buf->AvailableSize() > 0;
}
}  // namespace neodb