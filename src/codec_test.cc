#include "codec.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

namespace neodb {
class CodecTest : public ::testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CodecTest, EstimateEncodedBufferSize) {
//  uint32_t current_sz = 0;
//  // CRC(4B) + KEY LEN(2B) + VALUE LEN(4B) + 100 + 10 = 120
//  current_sz = Codec::EstimateEncodedBufferSize(current_sz, 10, 100);
//  EXPECT_EQ(current_sz, 120);
//  // 10 + 2 + 90 + 4
//  current_sz = Codec::EstimateEncodedBufferSize(current_sz, 10, 90);
//  EXPECT_EQ(current_sz, 226);
//  // 32522 + 4 + 10 + 2 + 226 = 32764
//  current_sz = Codec::EstimateEncodedBufferSize(current_sz, 10, 32522);
//  EXPECT_EQ(current_sz, 32764);
//  // 1 + 1 + 6 + 32764 + 4(next block CRC)= 32776
//  current_sz = Codec::EstimateEncodedBufferSize(current_sz, 1, 1);
//  EXPECT_EQ(current_sz, 32776);
//  // 32KB + 10 + 6 + 32776 + 4 =
//  current_sz = Codec::EstimateEncodedBufferSize(current_sz, 10, (32UL << 10));
//  EXPECT_EQ(current_sz, 65564);
//  // 10*32KB + 10*4 + 6 + 65564 + 10
//  current_sz =
//      Codec::EstimateEncodedBufferSize(current_sz, 10, 10 * (32UL << 10));
//  EXPECT_EQ(current_sz, 10 * (32UL << 10) + 10 * 4 + 6 + 65564 + 10);
}

int main(int argc, char** argv) {
  // testing::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}  // namespace neodb