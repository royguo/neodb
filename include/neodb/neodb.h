#pragma once

#define NEODB_VERSION "1.0.0"

namespace neodb {
  std::string GetVersion() { return NEODB_VERSION; }

  class NeoDB {
    public:
    NeoDB() {}
      ~NeoDB() {}
  };
}