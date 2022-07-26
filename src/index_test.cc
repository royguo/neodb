#include "neodb/index.h"

#include <gtest/gtest.h>

namespace neodb {
class IndexTest : public ::testing::Test {
  void SetUp() override {}

  void TearDown() override {}
};

  TEST_F(IndexTest, SimpleTest) {
    Index idx;

    // Test SSD Value
    idx.Put("key1", 10001ULL);
    Index::ValueType value = idx.Get("key1");
    ASSERT_TRUE(std::holds_alternative<Index::SSDValuePtr>(value));
    uint64_t ssd_value_ptr = std::get<Index::SSDValuePtr>(value);
    ASSERT_EQ(ssd_value_ptr, 10001ULL);

    // Test Memory Value
    //    std::shared_ptr<Slice> mem_ptr =
    //    std::make_shared<IOBuf>("memory_value"); idx.Put("key2", mem_ptr);
    //    value = idx.Get("key2");
    //    ASSERT_TRUE(std::holds_alternative<Index::MemoryValuePtr>(value));
    //    std::shared<IOBuf>& shared_value =
    //    std::get<Index::MemoryValuePtr>(value); ASSERT_EQ(shared_value.Data(),
    //    mem_ptr.Data());
  }

  int main(int argc, char** argv) {
    // testing::InstallFailureSignalHandler();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }
}  // namespace neodb
