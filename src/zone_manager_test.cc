#include "zone_manager.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "gtest/gtest.h"
#include "utils.h"

namespace neodb {
class ZoneManagerTest : public ::testing::Test {
 public:
  std::unique_ptr<ZoneManager> zone_manager_;
  std::string filename_;
  StoreOptions options_;
  std::shared_ptr<Index> index_ = std::make_shared<Index>();

  void SetUp() override {
    InitLogger();
    options_.writable_buffer_num_ = 1;
    options_.immutable_buffer_num_ = 1;
    options_.write_buffer_size_ = 1UL << 20;
    options_.device_zone_capacity_ = 50UL << 20;
    options_.device_capacity_ = 5UL << 30;
    options_.recover_exist_db_ = true;
    filename_ = FileUtils::GenerateRandomFile("zone_manager_test_file_", options_.device_capacity_);
    auto io_handle = std::make_unique<FileIOHandle>(filename_, options_.device_capacity_,
                                                    options_.device_zone_capacity_);

    zone_manager_ = std::make_unique<ZoneManager>(options_, std::move(io_handle), index_);
  }

  void TearDown() override { FileUtils::DeleteFile(filename_); }
};

TEST_F(ZoneManagerTest, ProcessSingleItemTest) {
  auto zone = zone_manager_->GetCurrentDataZone();
  // IO flush size set to 32KB
  std::shared_ptr<IOBuf> buffer = std::make_shared<IOBuf>(32UL << 10);
  char* ptr = buffer->Buffer();

  std::string key = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(10UL << 10));

  uint64_t lba = zone_manager_->TryFlushSingleItem(buffer, key, value, false);
  EXPECT_EQ(lba, zone->offset_ + 0);

  // total size = 6 + 10 + 10KB
  EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
  EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
  EXPECT_EQ(key, std::string(ptr + 6, key.size()));
  EXPECT_EQ(value->Data(), std::string(ptr + 6 + key.size(), value->Size()));
  ptr += 6 + 10 + (10UL << 10);

  // total size = 6 + 10 + 10KB + 6 + 2
  key = StringUtils::GenerateRandomString(1);
  value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(1));
  lba = zone_manager_->TryFlushSingleItem(buffer, key, value, false);
  EXPECT_EQ(lba, zone->offset_ + (10UL << 10) + 16);

  EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
  EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
  EXPECT_EQ(key, std::string(ptr + 6, key.size()));
  EXPECT_EQ(value->Data(), std::string(ptr + 6 + key.size(), value->Size()));
  ptr += 6 + 1 + 1;

  // total size = 6 + 10 + 10KB + 6 + 2 + 6 + 10 + 22KB
  key = StringUtils::GenerateRandomString(10);
  value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(22UL << 10));
  lba = zone_manager_->TryFlushSingleItem(buffer, key, value, false);
  LOG(INFO, "item appended");
  EXPECT_EQ(lba, zone->offset_ + (10UL << 10) + 16 + 6 + 2);
  EXPECT_EQ(key.size(), *reinterpret_cast<uint16_t*>(ptr));
  EXPECT_EQ(value->Size(), *reinterpret_cast<uint32_t*>(ptr + 2));
  EXPECT_EQ(key, std::string(ptr + 6, key.size()));
  EXPECT_EQ(buffer->Size(), 40);
  ptr = buffer->Buffer() + 40;

  key = StringUtils::GenerateRandomString(10);
  value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(64UL << 10));
  lba = zone_manager_->TryFlushSingleItem(buffer, key, value, false);
  EXPECT_EQ(lba, zone->offset_ + (10UL << 10) + 16 + 6 + 2 + (22UL << 10) + 16);
  LOG(INFO, "item appended");
  EXPECT_EQ(buffer->Size(), 56);
}

TEST_F(ZoneManagerTest, LargeWriteAndReadTest) {
  auto zone = zone_manager_->GetCurrentDataZone();

  std::shared_ptr<IOBuf> buffer = std::make_shared<IOBuf>(32UL << 10);
  // The item is larger than the IOBuffer capacity
  std::string key = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(100UL << 10));
  uint64_t lba = zone_manager_->TryFlushSingleItem(buffer, key, value, true);
  EXPECT_EQ(lba, zone->offset_ + 0);
  LOG(INFO, "100K item appended, lba offset: {}", lba);

  std::string read_key;
  std::shared_ptr<IOBuf> read_value;
  zone_manager_->ReadSingleItem(lba, &read_key, read_value);
  EXPECT_EQ(key, read_key);
  EXPECT_EQ(value->Data(), read_value->Data());
}

TEST_F(ZoneManagerTest, WriteAndReadTest) {
  auto zone = zone_manager_->GetCurrentDataZone();

  std::shared_ptr<IOBuf> buffer = std::make_shared<IOBuf>(32UL << 10);

  // Write two items
  std::string key1 = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value1 =
      std::make_shared<IOBuf>(StringUtils::GenerateRandomString(10UL << 10));
  uint64_t lba = zone_manager_->TryFlushSingleItem(buffer, key1, value1, false);
  EXPECT_EQ(lba, zone->offset_ + 0);
  LOG(INFO, "item appended, lba offset: {}", lba);

  std::string key2 = StringUtils::GenerateRandomString(10);
  std::shared_ptr<IOBuf> value2 = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(22496));
  lba = zone_manager_->TryFlushSingleItem(buffer, key2, value2, false);
  EXPECT_EQ(lba, zone->offset_ + (10UL << 10) + 16);
  LOG(INFO, "item appended, lba offset: {}", lba);

  // Read them out and verify
  std::shared_ptr<IOBuf> read_value1;
  std::string read_key1;
  zone_manager_->ReadSingleItem(zone->offset_ + 0, &read_key1, read_value1);
  EXPECT_EQ(read_key1, key1);
  EXPECT_EQ(read_value1->Data(), value1->Data());

  std::shared_ptr<IOBuf> read_value2;
  std::string read_key2;
  zone_manager_->ReadSingleItem(zone->offset_ + (10UL << 10) + 16, &read_key2, read_value2);
  EXPECT_EQ(read_key2, key2);
  EXPECT_EQ(read_value2->Data(), value2->Data());
}

TEST_F(ZoneManagerTest, TryFlushTest) {
  std::unordered_map<std::string, std::shared_ptr<IOBuf>> data;
  for (int i = 0; i < 15; ++i) {
    std::string key = StringUtils::GenerateRandomString(10);
    auto value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(100UL << 10));
    auto s = zone_manager_->Append(key, value);
    EXPECT_TRUE(s.ok());
    data.emplace(key, value);
    index_->Put(key, value);
  }
  EXPECT_EQ(1, zone_manager_->GetImmutableBufferNum());
  EXPECT_EQ(1, zone_manager_->GetWritableBufferNum());

  // Check Key Value
  for (auto& item : data) {
    Index::ValueVariant value;
    auto s = index_->Get(item.first, value);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(std::get<Index::MemValue>(value)->Data(), item.second->Data());
  }

  // After flush, the immutable_ buffer should be consumed.
  zone_manager_->FlushImmutableBuffers();
  EXPECT_EQ(0, zone_manager_->GetImmutableBufferNum());

  //   Check Key Value
  for (auto& item : data) {
    Index::ValueVariant value;
    auto s = index_->Get(item.first, value);
    EXPECT_TRUE(s.ok());
    if (std::holds_alternative<Index::MemValue>(value)) {
      EXPECT_EQ(std::get<Index::MemValue>(value)->Data(), item.second->Data());
    } else {
      uint64_t lba = std::get<Index::LBAValue>(value);
      std::shared_ptr<IOBuf> ssd_value;
      std::string ssd_key;
      s = zone_manager_->ReadSingleItem(lba, &ssd_key, ssd_value);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(item.first, ssd_key);
      EXPECT_EQ(ssd_value->Data(), item.second->Data());
    }
  }
}

TEST_F(ZoneManagerTest, AppendToActiveZoneTest) {
  // Start the background flush thread first
  zone_manager_->StartFlushWorker();

  // Then we insert 1GB data to the system
  std::unordered_map<std::string, std::shared_ptr<IOBuf>> data;
  uint64_t written_bytes = 0;
  for (int i = 0; i < (2UL << 10); ++i) {
    std::string key = StringUtils::GenerateRandomString(10);
    auto value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(100UL << 10));
    auto s = zone_manager_->Append(key, value);
    EXPECT_TRUE(s.ok());
    data.emplace(key, value);
    index_->Put(key, value);
    written_bytes += (100UL << 10) + 10 + 6;
  }
  LOG(INFO, "Total written bytes about: {}", written_bytes);

  zone_manager_->StopFlushWorker();
}

TEST_F(ZoneManagerTest, ExceedsDeviceCapacityTest) {
  // 10 zones in total
  options_.device_capacity_ = 100UL << 20;
  options_.device_zone_capacity_ = 20UL << 20;
  options_.gc_threshold_zone_num_ = 1;

  LOG(INFO, "resize device and zone size....");
  // reset zone_manager
  auto io_handle = std::make_unique<FileIOHandle>(filename_, options_.device_capacity_,
                                                  options_.device_zone_capacity_);
  zone_manager_ = std::make_unique<ZoneManager>(options_, std::move(io_handle), index_);
  zone_manager_->StartFlushWorker();
  zone_manager_->StartGCWorker();

  // write 10 zones in total will trigger GC, about 200MB
  for (uint64_t sz = 0; sz < (200UL << 20); ++sz) {
    std::string key = StringUtils::GenerateRandomString(10);
    auto value = std::make_shared<IOBuf>(StringUtils::GenerateRandomString(1UL << 20));
    auto s = zone_manager_->Append(key, value);
    EXPECT_TRUE(s.ok());
    index_->Put(key, value);
    sz += (value->Size() + 10 + 6);
  }

  zone_manager_->StopFlushWorker();
  zone_manager_->StopGCWorker();
}

TEST_F(ZoneManagerTest, LayoutTest) {
  // one single active zone for this device
  options_.device_capacity_ = 100UL << 20;
  options_.device_zone_capacity_ = 80UL << 20;

  LOG(INFO, "resize device and zone size....");
  // reset zone_manager
  auto io_handle = std::make_unique<FileIOHandle>(filename_, options_.device_capacity_,
                                                  options_.device_zone_capacity_);
  zone_manager_ = std::make_unique<ZoneManager>(options_, std::move(io_handle), index_);
  auto io_buffer = std::make_shared<IOBuf>((32 << 10));

  uint64_t total_bytes = 0;
  uint64_t now = TimeUtils::GetCurrentTimeInSeconds();

  while (total_bytes < options_.device_zone_capacity_) {
    std::vector<std::string> keys;
    std::vector<std::shared_ptr<IOBuf>> values;
    std::vector<uint64_t> lbas;
    int batch_size = 32;
    for (int i = 0; i < batch_size; ++i) {
      std::string key = StringUtils::GenerateRandomString(NumberUtils::FastRand16() % MAX_KEY_SIZE);
      std::string value_str =
          StringUtils::GenerateRandomString(NumberUtils::FastRand32() % MAX_KEY_SIZE);
      auto value = std::make_shared<IOBuf>(value_str);
      keys.emplace_back(key);
      values.emplace_back(value);
      uint64_t lba = zone_manager_->TryFlushSingleItem(io_buffer, key, value);
      lbas.emplace_back(lba);
      total_bytes += (key.size() + value->Size());
      if (TimeUtils::GetCurrentTimeInSeconds() - now >= 1) {
        LOG(INFO, "total write: {} MB", total_bytes >> 20);
        now = TimeUtils::GetCurrentTimeInSeconds();
      }
    }

    // Flush buffer each 2 items and then read them out to verify
    zone_manager_->FlushAndResetIOBuffer(io_buffer);
    EXPECT_TRUE(keys.size() == batch_size);
    for (int i = 0; i < batch_size; ++i) {
      std::string read_key;
      std::shared_ptr<IOBuf> read_value;
      zone_manager_->ReadSingleItem(lbas[i], &read_key, read_value);
      EXPECT_EQ(read_key, keys[i]);
      EXPECT_EQ(read_value->Data(), values[i]->Data());
    }
  }
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
