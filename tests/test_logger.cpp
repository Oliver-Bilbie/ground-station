#include <gtest/gtest.h>
#include "globals.h"
#include "logger.h"

class LoggerTest : public testing::Test {
 protected:
  Logger logger;
  uint64_t last_printed() { return logger.last_printed; }
  size_t buffer_size() { return logger.buffer.size(); }
  uint64_t timeout_cycles = TIMEOUT_MS / CYCLE_LENGTH_MS;
};

TEST_F(LoggerTest, LogImmediatelyWhenReceivedInOrder) {
  for (int i = 1; i <= 10; i++) {
    logger.log(PositionPacketData(i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(), 0);
    EXPECT_EQ(last_printed(), i);
  }
}

TEST_F(LoggerTest, LogOnceGapIsFilled) {
  for (int i = 2; i <= timeout_cycles; i++) {
    logger.log(PositionPacketData(i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(), i - 1);
    EXPECT_EQ(last_printed(), 0);
  }

  logger.log(PositionPacketData(1, 1, 1, 1, 1));
  EXPECT_EQ(buffer_size(), 0);
  EXPECT_EQ(last_printed(), timeout_cycles);
}

TEST_F(LoggerTest, LogOnceGapIsTimedOut) {
  for (int i = 2; i <= timeout_cycles; i++) {
    logger.log(PositionPacketData(i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(), i - 1);
    EXPECT_EQ(last_printed(), 0);
  }

  logger.log(PositionPacketData(timeout_cycles + 1, 1, 1, 1, 1));
  EXPECT_EQ(buffer_size(), 0);
  EXPECT_EQ(last_printed(), timeout_cycles + 1);
}
