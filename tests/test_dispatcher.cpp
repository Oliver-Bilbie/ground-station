#include <gtest/gtest.h>
#include "dispatcher.h"

class DispatcherTest : public testing::Test {
 protected:
  Dispatcher dispatcher{-1};

  bool is_retrying(uint64_t packet_num) {
    std::lock_guard<std::mutex> lock(dispatcher.mtx);
    return dispatcher.retry_counts.count(packet_num) > 0;
  }

  size_t retry_count_size() {
    std::lock_guard<std::mutex> lock(dispatcher.mtx);
    return dispatcher.retry_counts.size();
  }
};

TEST_F(DispatcherTest, ReceiveInOrderDoesNotTriggerRetry) {
  dispatcher.receive(1);
  EXPECT_EQ(retry_count_size(), 0);

  dispatcher.receive(2);
  EXPECT_EQ(retry_count_size(), 0);
}

TEST_F(DispatcherTest, DetectsSingleGap) {
  dispatcher.receive(1);
  dispatcher.receive(3);

  ASSERT_EQ(retry_count_size(), 1);
  EXPECT_FALSE(is_retrying(1));
  EXPECT_TRUE(is_retrying(2));
  EXPECT_FALSE(is_retrying(3));
}

TEST_F(DispatcherTest, DetectsMultipleGaps) {
  dispatcher.receive(1);
  dispatcher.receive(5);

  ASSERT_EQ(retry_count_size(), 3);
  EXPECT_FALSE(is_retrying(1));
  EXPECT_TRUE(is_retrying(2));
  EXPECT_TRUE(is_retrying(3));
  EXPECT_TRUE(is_retrying(4));
  EXPECT_FALSE(is_retrying(5));
}

TEST_F(DispatcherTest, FillsGapSuccessfully) {
  dispatcher.receive(1);
  dispatcher.receive(3);

  ASSERT_TRUE(is_retrying(2));

  dispatcher.receive(2);

  ASSERT_FALSE(is_retrying(2));
}

TEST_F(DispatcherTest, IgnoresDuplicates) {
  dispatcher.receive(1);
  dispatcher.receive(1);

  EXPECT_EQ(retry_count_size(), 0);
}
