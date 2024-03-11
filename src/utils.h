#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "logger.h"

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
  static std::string GenerateRandomFile(const std::string& prefix,
                                        uint32_t size) {
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
    if (std::filesystem::exists(filename) &&
        std::filesystem::is_regular_file(filename)) {
      std::filesystem::remove(filename);
      return true;
    }
    return false;
  }

  // Delete all files in target directory
  static int DeleteFilesByPrefix(const std::string& dir,
                                 const std::string& prefix) {
    int total = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.is_regular_file() &&
          entry.path().filename().string().find(prefix) == 0) {
        std::filesystem::remove(entry);
        total++;
      }
    }
    return total;
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

  static uint64_t AlignTo(uint64_t num, uint64_t alignment) {
    if (alignment <= 1) return num;
    if (num < alignment) return alignment;
    return (num + alignment - 1) / alignment * alignment;
  }
};

class HashUtils {
 public:
  // TODO(Roy Guo) Use a simple hash & fast hash library
  static uint64_t FastHash(const std::string& str) { return 1; }
};

class TimeUtils {};
}  // namespace neodb