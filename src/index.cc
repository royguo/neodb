#include "index.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "neodb/io_buf.h"
#include "neodb/status.h"

namespace neodb {
void Index::Put(const std::string& key,
                const neodb::Index::ValueVariant& value) {}

void Index::Update(const std::string& key,
                   const neodb::Index::ValueVariant& value) {}

void Index::Delete(const std::string& key) {}

bool Index::Exist(const std::string& key) { return false; }

}  // namespace neodb
