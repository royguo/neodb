#include "zone_manager.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "codec.h"
#include "utils.h"

namespace neodb {

#define FLUSH_IO_SIZE (32UL << 10)
#define SECTOR_SIZE (512)

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

  uint16_t meta_sz = 6;  // 2B key len + 4B value len
  auto* items = immutable->GetItems();
  // iterate all kv items and encode them to the IO buffer, if the buffer
  // exceeds IO limit (e.g. 256KB), flush it.
  for (auto& item : *items) {
    uint16_t key_sz = item.first.size();
    uint32_t value_sz = item.second->Size();
    // We should use a new empty buffer if there's no enough free space.
    if (available_sz <= (key_sz + meta_sz)) {
      //      io_handle_->Write();
      available_sz = FLUSH_IO_SIZE;
      cur_ptr = ptr;
      encoded_buf->Reset();
    }

    // append meta and key data
    memcpy(cur_ptr, &key_sz, 2);
    memcpy(cur_ptr + 2, &value_sz, 4);
    memcpy(cur_ptr + 6, item.first.c_str(), key_sz);
    cur_ptr += (6 + key_sz);
    available_sz -= (6 + key_sz);

    // append value data
    auto* value_ptr = item.second->Buffer();
    auto* value_cur_ptr = value_ptr;

    while (value_cur_ptr - value_ptr != value_sz) {
      uint32_t sz = std::min(available_sz,
                             value_sz - uint32_t(value_cur_ptr - value_ptr));
      memcpy(cur_ptr, value_cur_ptr, sz);
      cur_ptr += sz;
      value_cur_ptr += sz;
      available_sz -= sz;

      // If there's no more reminding value data, we should stop here
      if (value_cur_ptr - value_ptr == value_sz && available_sz > 0) {
        break;
      }
      // if there's still more value data, we should flush current buffer and
      // continue.
      //      io_handle_->Write()
      if (available_sz == 0) {
        encoded_buf->Reset();
        cur_ptr = ptr;
        available_sz = IO_BLOCK_SIZE;
      }
    }
    // If all items were consumed but there still have some data in the buffer,
    // we should flush them too.
    if (cur_ptr - ptr > 0) {
      //      io_handle_->Write()
    }
  }

  // TODO(Roy Guo) update related index, point the index to LBA address
}

bool ZoneManager::ProcessSingleItem(
    std::shared_ptr<IOBuf> buf, const std::string& key,
    std::shared_ptr<IOBuf> value,
    std::function<void(std::shared_ptr<IOBuf>, bool)> func) {
  const uint16_t meta_sz = 6;  // 2B key len + 4B value len
  const uint16_t key_sz = key.size();
  const uint32_t value_sz = value->Size();
  // We should process item buffer and then reset it if there's no enough free
  // space for key data and meta
  if (buf->AvailableSize() <= (key_sz + meta_sz)) {
    func(buf, true /* reset buffer */);
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
    func(buf, buf->AvailableSize() == 0 /* reset buffer */);
  }
  return buf->AvailableSize() > 0;
}
}  // namespace neodb