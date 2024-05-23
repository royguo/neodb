#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "aio_engine.h"
#include "logger.h"
#include "neodb/tiny_dir.h"

namespace neodb {
class StringUtils {
 public:
  static std::string GenerateRandomString(uint32_t len) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint32_t> dist;
    char buf[len];
    for (auto i = 0; i < len; ++i) {
      buf[i] = 'a' + dist(gen) % 26;
    }
    return {buf, len};
  }
};

class FileUtils {
 public:
  static std::string GenerateRandomFile(const std::string& prefix, uint32_t size) {
    std::string filename = prefix + std::to_string(std::random_device()());
    LOG(INFO, "Generate random file, filename: {}", filename);
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
      LOG(ERROR, "Open writable file failed, error: {}", std::strerror(errno));
      abort();
    }

    int ret = ftruncate(fd, size);
    if (ret == -1) {
      LOG(ERROR, "ftruncate file failed : {}", std::strerror(errno));
    }
    return filename;
  }

  static bool DeleteFile(const std::string& filename) {
    if (FileExist(filename)) {
      return std::remove(filename.c_str()) == 0;
    }
    return false;
  }

  static bool FileExist(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
  }

  // Delete all files in target directory
  static int DeleteFilesByPrefix(const std::string& dir, const std::string& prefix) {
    tinydir_dir d;
    tinydir_open(&d, dir.c_str());
    int total = 0;
    while (d.has_next) {
      tinydir_file file;
      tinydir_readfile(&d, &file);

      if (!file.is_dir && strncmp(file.name, prefix.c_str(), prefix.size()) == 0) {
        std::remove(file.name);
        total++;
      }
      tinydir_next(&d);
    }
    tinydir_close(&d);
    return total;
  }

  static int OpenDirectFile(const std::string& filename, bool read_only = false) {
    int fd_ = -1;
#ifdef __APPLE__
    fd_ =
        open(filename.c_str(), read_only ? O_RDONLY : O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
    fcntl(fd_, F_NOCACHE, 1);
#else
    fd_ = open(filename.c_str(), read_only ? O_RDONLY : O_RDWR | O_DIRECT | O_CREAT,
               S_IRUSR | S_IWUSR);
#endif
    return fd_;
  }
};

class IOEngineUtils {
 public:
  static AIOEngine* GetInstance() {
    AIOEngine* engine;
#ifdef __APPLE__
    engine = new MockAIOEngine();
#else
    engine = new PosixAIOEngine();
#endif
    return engine;
  }
};

class NumberUtils {
 public:
  static int FastRand16() {
    // Compute a pseudorandom integer, Output value in range [0, 32767]
    static unsigned int g_seed = 1988;
    g_seed = (214013 * g_seed + 2531011);
    return (g_seed >> 16) & 0x7FFF;
  }

  // Get random uint64 integer
  static uint64_t FastRand64() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dist;
    return dist(gen);
  }

  static uint64_t FastRand32() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint32_t> dist;
    return dist(gen);
  }

  static uint64_t AlignTo(uint64_t num, uint64_t alignment) {
    if (alignment <= 1) return num;
    if (num < alignment) return alignment;
    return (num + alignment - 1) / alignment * alignment;
  }
};

class HashUtils {
 public:
  inline static uint64_t FastHash(const std::string& str) {
    uint64_t h = 0;
    for (char c : str) {
      h = (h << 5) + h + c;
    }
    return h;
  }
};

class TimeUtils {
 public:
  inline static uint64_t GetCurrentTimeInNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  inline static uint64_t GetCurrentTimeInUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  inline static uint64_t GetCurrentTimeInSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
};
}  // namespace neodb