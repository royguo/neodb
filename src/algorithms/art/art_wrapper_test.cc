#include "art_wrapper.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "art.h"

namespace neodb {
class ARTWrapperTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ARTWrapperTest, SimpleRawAPITest) {
  art_tree tree;
  art_tree_init(&tree);

  for (int i = 0; i < 100; ++i) {
    std::string key = "key" + std::to_string(i);
    int key_len = (int)key.size();
    const auto* key_ptr = reinterpret_cast<const unsigned char*>(key.data());
    int value = 1001 + i;
    art_insert(&tree, key_ptr, key_len, &value);

    uint64_t ret_value_ptr = (art_search(&tree, key_ptr, key_len));
    int ret_value = *(int*)ret_value_ptr;
    EXPECT_EQ(ret_value, value);
  }
  art_tree_destroy(&tree);
}

TEST_F(ARTWrapperTest, SimpleWrapperTest) {
  ARTWrapper wrapper;
  wrapper.Put("key1", 1001);
  uint64_t value = 0;
  Status s = wrapper.Get("key1", &value);
  EXPECT_EQ(value, 1001);
  EXPECT_TRUE(s.ok());
  EXPECT_TRUE(wrapper.Exist("key1"));

  s = wrapper.Get("key_not_found", &value);
  EXPECT_TRUE(!s.ok());

  wrapper.Put("key1", 1002);
  s = wrapper.Get("key1", &value);
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(value, 1002);

  wrapper.Delete("key1");
  s = wrapper.Get("key1", &value);
  EXPECT_TRUE(!s.ok());
  EXPECT_FALSE(wrapper.Exist("key1"));
}

TEST_F(ARTWrapperTest, ZeroTest) {
  ARTWrapper wrapper;
  auto s = wrapper.Put("key1", 0);
  ASSERT_TRUE(s.ok());

  uint64_t value = 9999;
  s = wrapper.Get("key1", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, 0);
}
}  // namespace neodb