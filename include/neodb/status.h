#pragma once
#include <string>

namespace neodb {
class Status {
 public:
  enum Code { kOK = 0, kNotFound, kIOError, kIOBusy };

 public:
  static Status OK(const std::string& msg = "") { return {Code::kOK, msg}; }

  static Status NotFound(const std::string& msg = "") {
    return {Code::kNotFound, msg};
  }

  static Status Busy(const std::string& msg = "") {
    return {Code::kIOBusy, msg};
  }

  static Status IOError(const std::string& msg = "") {
    return {Code::kIOError, msg};
  }

  static Status StatusError(const std::string& msg = "status not valid") {
    return {Code::kIOError, msg};
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
}  // namespace neodb
