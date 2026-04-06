#include <gtest/gtest.h>
#include <chrono>
#include "gps.h"

const double earth_rad_m = 6371000;
const double leo_rad_m_min = earth_rad_m + 500000;
const double leo_rad_m_max = earth_rad_m + 600000;
const double leo_rad_m_2_min = leo_rad_m_min * leo_rad_m_min;
const double leo_rad_m_2_max = leo_rad_m_max * leo_rad_m_max;

struct DummyClock {
  using time_point = std::chrono::system_clock::time_point;

 public:
  time_point now() const { return dummy_time; }
  void set_time_minutes(int m) {
    dummy_time = std::chrono::system_clock::time_point{std::chrono::minutes(m)};
    ;
  };

 private:
  time_point dummy_time;
};

bool is_in_leo(Position p) {
  double r_2 = p.x * p.x + p.y * p.y + p.z * p.z;
  return r_2 >= leo_rad_m_2_min && r_2 <= leo_rad_m_2_max;
}

class GPSTest : public testing::Test {
 protected:
  DummyClock clock;
  GPS<DummyClock> gps{clock};
};

TEST_F(GPSTest, PositionIsValidOnInit) {
  clock.set_time_minutes(0);
  Position p = gps.get_position();
  ASSERT_TRUE(is_in_leo(p));
}

TEST_F(GPSTest, PositionIsValidAfterOneMinute) {
  clock.set_time_minutes(1);
  Position p = gps.get_position();
  ASSERT_TRUE(is_in_leo(p));
}

TEST_F(GPSTest, PositionIsValidAfterOneHour) {
  clock.set_time_minutes(60);
  Position p = gps.get_position();
  ASSERT_TRUE(is_in_leo(p));
}

TEST_F(GPSTest, PositionIsValidAfterOneYear) {
  clock.set_time_minutes(525600);
  Position p = gps.get_position();
  ASSERT_TRUE(is_in_leo(p));
}
