#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "timer.h"

class TimerTest : public testing::Test {
 protected:
  Timer timer;
};

TEST_F(TimerTest, CorrectElapsedTime) {
  const int interval = 10;

  timer.reset();
  EXPECT_NEAR(timer.elapsed(), 0, 1);

  for (int i = 1; i <= 5; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    EXPECT_NEAR(timer.elapsed(), interval * i, 1);
  }
}
