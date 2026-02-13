#ifndef GPS_H
#define GPS_H

#include <chrono>
#include <cmath>

struct Position {
  double x;
  double y;
  double z;
  uint64_t timestamp;
};

struct State {
  double x, y, z;
  double vx, vy, vz;
  uint64_t last_update_time_ms;
};

struct Derivative {
  double dx, dy, dz;
  double dvx, dvy, dvz;
};

struct SystemClock {
  std::chrono::system_clock::time_point now() const {
    return std::chrono::system_clock::now();
  }
};

template <typename Clock = SystemClock>
class GPS {
 public:
  GPS() {
    SystemClock clock;
    _clock = clock;
    setup();
  }
  GPS(Clock& clock) : _clock(clock) { setup(); }

  Position get_position() {
    update_position();
    return Position{state.x, state.y, state.z, state.last_update_time_ms};
  }

 private:
  const double GM = 3.986004418e14;       // Earth's Gravitational Parameter (m^3/s^2)
  const double EARTH_RADIUS = 6371000.0;  // Meters
  const double ALTITUDE = 550000.0;       // 550km
  State state;
  Clock _clock;

  void setup() {
    double r = EARTH_RADIUS + ALTITUDE;
    double v = std::sqrt(GM / r);

    double theta = 45.0 * (M_PI / 180.0);
    double inc = 53.0 * (M_PI / 180.0);

    state.x = r * std::cos(theta);
    state.y = r * std::sin(theta) * std::cos(inc);
    state.z = r * std::sin(theta) * std::sin(inc);

    state.vx = -v * std::sin(theta);
    state.vy = v * std::cos(theta) * std::cos(inc);
    state.vz = v * std::cos(theta) * std::sin(inc);

    state.last_update_time_ms = get_current_time_ms();
  }

  uint64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               _clock.now().time_since_epoch())
        .count();
  }

  Derivative evaluate(const State& initial, double t, double dt, const Derivative& d) {
    State s;
    s.x = initial.x + d.dx * dt;
    s.y = initial.y + d.dy * dt;
    s.z = initial.z + d.dz * dt;
    s.vx = initial.vx + d.dvx * dt;
    s.vy = initial.vy + d.dvy * dt;
    s.vz = initial.vz + d.dvz * dt;

    double r = std::sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
    double r3 = r * r * r;

    Derivative output;
    output.dx = s.vx;
    output.dy = s.vy;
    output.dz = s.vz;
    output.dvx = -GM * s.x / r3;
    output.dvy = -GM * s.y / r3;
    output.dvz = -GM * s.z / r3;

    return output;
  }

  void update_position() {
    uint64_t now_ms = get_current_time_ms();
    double dt = (now_ms - state.last_update_time_ms) / 1000.0;  // Convert to seconds

    if (dt <= 0)
      return;  // No time passed

    // Runge-Kutta 4 Integration
    Derivative k1 = evaluate(state, 0.0, 0.0, Derivative());
    Derivative k2 = evaluate(state, 0.0, dt * 0.5, k1);
    Derivative k3 = evaluate(state, 0.0, dt * 0.5, k2);
    Derivative k4 = evaluate(state, 0.0, dt, k3);

    double dxdt = (1.0 / 6.0) * (k1.dx + 2.0 * (k2.dx + k3.dx) + k4.dx);
    double dydt = (1.0 / 6.0) * (k1.dy + 2.0 * (k2.dy + k3.dy) + k4.dy);
    double dzdt = (1.0 / 6.0) * (k1.dz + 2.0 * (k2.dz + k3.dz) + k4.dz);

    state.x += dxdt * dt;
    state.y += dydt * dt;
    state.z += dzdt * dt;

    double dvxdt = (1.0 / 6.0) * (k1.dvx + 2.0 * (k2.dvx + k3.dvx) + k4.dvx);
    double dvydt = (1.0 / 6.0) * (k1.dvy + 2.0 * (k2.dvy + k3.dvy) + k4.dvy);
    double dvzdt = (1.0 / 6.0) * (k1.dvz + 2.0 * (k2.dvz + k3.dvz) + k4.dvz);

    state.vx += dvxdt * dt;
    state.vy += dvydt * dt;
    state.vz += dvzdt * dt;

    state.last_update_time_ms = now_ms;
  }
};

#endif
