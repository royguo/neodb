#include "neodb/neodb.h"

#include "neodb/index.h"
#include "neodb/write_buffer.h"

namespace neodb {
Status NeoDB::Put(const std::string& key, std::shared_ptr<IOBuf> value) {
  write_buffer_->PushOrWait(key, value);
  return Status::OK();
}

Status NeoDB::Get(const std::string& key, std::shared_ptr<IOBuf>* value) {
  return Status::OK();
}
}  // namespace neodb
