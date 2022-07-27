#include "neodb/index.h"

#include <gtest/gtest.h>

#include <memory>

namespace neodb {
class IndexTest : public ::testing::Test {
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(IndexTest, SimpleTest) {
  Index idx;

  // Test SSD Value Pointer
  uint64_t lba = 10001ULL;
  idx.Put("key1", lba);
  Index::ValueType value = idx.Get("key1");
  ASSERT_TRUE(std::holds_alternative<Index::SSDValuePtr>(value));
  uint64_t ssd_value_ptr = std::get<Index::SSDValuePtr>(value);
  ASSERT_EQ(ssd_value_ptr, lba);

  // Test Memory Value
  std::string memory_data = "memory_data";
  auto original_shared_value = std::make_shared<IOBuf>(std::move(memory_data));
  idx.Put("key2", original_shared_value);
  auto get_value_type = idx.Get("key2");
  ASSERT_TRUE(std::holds_alternative<Index::MemoryValuePtr>(get_value_type));
  auto get_shared_value = std::get<Index::MemoryValuePtr>(get_value_type);
  ASSERT_EQ(get_shared_value->Data(), "memory_data");

  // Test not found memory value
  get_value_type = idx.Get("key_not_found");
  ASSERT_TRUE(std::holds_alternative<Index::SSDValuePtr>(get_value_type));
  uint64_t not_found_ptr = std::get<Index::SSDValuePtr>(get_value_type);
  ASSERT_EQ(not_found_ptr, Index::kIndexNotFound);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
}  // namespace neodb
