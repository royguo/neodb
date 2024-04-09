#include "concurrent_index.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>
#include <thread>

#include "logger.h"
#include "neodb/histogram.h"
#include "utils.h"

namespace neodb {
class ConcurrentIndexTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ConcurrentIndexTest, ArtIndexTest) {
  LOG(INFO, "ArtIndexTest...");
  auto t1 = TimeUtils::GetCurrentTimeInUs();
  int concurrency = 20;
  int items_per_worker = 1000;
  ConcurrentArt art(concurrency);
  std::vector<std::thread> workers;
  workers.reserve(concurrency);
  for (int i = 0; i < concurrency; ++i) {
    workers.emplace_back([&, i]() {
      for (int j = 0; j < items_per_worker; ++j) {
        std::string key = "key" + std::to_string(i) + " " + std::to_string(j);
        uint64_t value = j;
        auto s = art.Put(key, value);
        EXPECT_TRUE(s.ok());
        s = art.Get(key, &value);
        EXPECT_TRUE(s.ok());
        EXPECT_EQ(value, j);
      }
    });
  }

  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  for (int i = 0; i < concurrency; ++i) {
    for (int j = 0; j < items_per_worker; ++j) {
      std::string key = "key" + std::to_string(i) + " " + std::to_string(j);
      uint64_t value = 0;
      auto s = art.Get(key, &value);
      EXPECT_TRUE(s.ok()) << "key: " << key;
      EXPECT_EQ(value, j);
    }
  }
  auto t2 = TimeUtils::GetCurrentTimeInUs();
  LOG(INFO, "ART Index Time Cost: {} us", t2 - t1);
}

TEST_F(ConcurrentIndexTest, HashMapIndexTest) {
  LOG(INFO, "HashMapIndexTest...");
  auto t1 = TimeUtils::GetCurrentTimeInUs();
  int concurrency = 20;
  int items_per_worker = 1000;
  ConcurrentHashMap hashmap(concurrency);
  std::vector<std::thread> workers;
  workers.reserve(concurrency);
  for (int i = 0; i < concurrency; ++i) {
    workers.emplace_back([&, i]() {
      for (int j = 0; j < items_per_worker; ++j) {
        std::string key = "key" + std::to_string(i) + " " + std::to_string(j);
        auto value = std::make_shared<IOBuf>(std::to_string(j));
        auto s = hashmap.Put(key, value);
        EXPECT_TRUE(s.ok());
        s = hashmap.Get(key, value);
        EXPECT_TRUE(s.ok());
        EXPECT_EQ(value->Data(), std::to_string(j));
      }
    });
  }

  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  auto t2 = TimeUtils::GetCurrentTimeInUs();

  for (int i = 0; i < concurrency; ++i) {
    for (int j = 0; j < items_per_worker; ++j) {
      std::string key = "key" + std::to_string(i) + " " + std::to_string(j);
      std::shared_ptr<IOBuf> value;
      auto s = hashmap.Get(key, value);
      EXPECT_TRUE(s.ok()) << "key: " << key;
      EXPECT_EQ(value->Data(), std::to_string(j));
    }
  }
  auto t3 = TimeUtils::GetCurrentTimeInUs();
  LOG(INFO, "HashMap Index Time Cost, insertion: {} us, checking: {} us", t2 - t1, t3 - t2);
}
}  // namespace neodb