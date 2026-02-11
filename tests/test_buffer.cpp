#include <gtest/gtest.h>
#include "buffer.h"

class BufferTest : public testing::Test {
 protected:
  Buffer<int> buffer;

  BufferTest() : buffer(10) {
    buffer.push(1);
    buffer.push(2);
  }
};

TEST_F(BufferTest, GetMostRecentReturnsLastPushed) {
  auto result = buffer.get(0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 2);
}

TEST_F(BufferTest, GetPreviousReturnsFirstPushed) {
  auto result = buffer.get(1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 1);
}

TEST_F(BufferTest, GetOutOfBoundsReturnsNullopt) {
  auto result = buffer.get(3);
  EXPECT_FALSE(result.has_value());
}

TEST_F(BufferTest, GetNegativeReturnsNullopt) {
  auto result = buffer.get(-1);
  EXPECT_FALSE(result.has_value());
}
