#ifndef GPS_H
#define GPS_H

#include <chrono>

struct Position {
  double x;
  double y;
  double z;
  uint64_t timestamp;
};

struct State {
  double x, y, z;
  double vx, vy, vz;
};

struct Derivative {
  double dx, dy, dz;
  double dvx, dvy, dvz;
};

class GPS {
 public:
  State state;
  uint64_t last_update_time_ms;

  GPS();
  uint64_t get_current_time_ms();
  Derivative evaluate(const State& initial, double t, double dt, const Derivative& d);
  void update_position();
  Position get_position();

 private:
  const double GM = 3.986004418e14;       // Earth's Gravitational Parameter (m^3/s^2)
  const double EARTH_RADIUS = 6371000.0;  // Meters
  const double ALTITUDE = 550000.0;       // 550km
};

#endif
