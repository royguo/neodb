#pragma once

namespace neodb {
    class Status {
     public:
      enum Code { kOK = 0, kNotFound, kIOError };

     public:
      static Status OK() { return Status(Code::kOK); }

     public:
      Status(Code code) : code_(code) {}

      bool ok() { return code_ == kOK; }

     private:
      Code code_ = Cdde::kOK;
    };
}