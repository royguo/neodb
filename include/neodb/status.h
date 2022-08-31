#pragma once
#include <string>

namespace neodb {
class Status {
 public:
  enum Code { kOK = 0, kNotFound, kIOError };

 public:
  static Status OK(const std::string& msg = "") {
    return Status(Code::kOK, msg);
  }

  static Status NotFound(const std::string& msg = "") {
    return Status(Code::kNotFound, msg);
  }

  static Status IOError(const std::string& msg = "") {
    return Status(Code::kIOError, msg);
  }

 public:
  Status(Code code) : code_(code) {}
  Status(Code code, const std::string& msg) : code_(code), msg_(msg) {}

  bool ok() { return code_ == kOK; }

  std::string msg() { return msg_; }

 private:
  Code code_ = Code::kOK;

  std::string msg_;
};
}
