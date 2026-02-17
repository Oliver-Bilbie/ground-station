#include <gtest/gtest.h>
#include "globals.h"
#include "logger.h"

class LoggerTest : public testing::Test {
 protected:
  Logger logger;
  uint64_t last_printed(uint64_t satellite_id) {
    return logger.last_printed[satellite_id];
  }
  size_t buffer_size(uint64_t satellite_id) {
    return logger.buffers[satellite_id].size();
  }
  uint64_t timeout_cycles = TIMEOUT_MS / CYCLE_LENGTH_MS;
};

TEST_F(LoggerTest, LogImmediatelyWhenReceivedInOrder) {
  for (int i = 1; i <= 10; i++) {
    logger.log(PositionPacketData(1, i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(1), 0);
    EXPECT_EQ(last_printed(1), i);
  }
}

TEST_F(LoggerTest, LogOnceGapIsFilled) {
  for (int i = 2; i <= timeout_cycles; i++) {
    logger.log(PositionPacketData(1, i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(1), i - 1);
    EXPECT_EQ(last_printed(1), 0);
  }

  logger.log(PositionPacketData(1, 1, 1, 1, 1, 1));
  EXPECT_EQ(buffer_size(1), 0);
  EXPECT_EQ(last_printed(1), timeout_cycles);
}

TEST_F(LoggerTest, LogOnceGapIsTimedOut) {
  for (int i = 2; i <= timeout_cycles; i++) {
    logger.log(PositionPacketData(1, i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(1), i - 1);
    EXPECT_EQ(last_printed(1), 0);
  }

  logger.log(PositionPacketData(1, timeout_cycles + 1, 1, 1, 1, 1));
  EXPECT_EQ(buffer_size(1), 0);
  EXPECT_EQ(last_printed(1), timeout_cycles + 1);
}

TEST_F(LoggerTest, HandleMultipleSatellites) {
  for (int i = 1; i <= 10; i++) {
    logger.log(PositionPacketData(1, i, 1, 1, 1, 1));
    logger.log(PositionPacketData(2, i, 1, 1, 1, 1));
    EXPECT_EQ(buffer_size(1), 0);
    EXPECT_EQ(last_printed(1), i);
    EXPECT_EQ(buffer_size(2), 0);
    EXPECT_EQ(last_printed(2), i);
  }
}
