#pragma once

namespace neodb {
// Slice doesn't own the underlying memory space.
class Slice {
 public:
  Slice() {}
  ~Slice() = default;
};
}