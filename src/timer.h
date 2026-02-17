#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class Timer {
 public:
  Timer() : cycle_start(std::chrono::system_clock::now()) {};

  void set(std::chrono::time_point<std::chrono::system_clock> t) { cycle_start = t; };

  void set(uint64_t timestamp) {
    cycle_start = std::chrono::system_clock::from_time_t(0) +
                  std::chrono::milliseconds(timestamp);
  }

  void reset() { cycle_start = std::chrono::system_clock::now(); };

  long long elapsed() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now() - cycle_start)
        .count();
  };

 private:
  std::chrono::time_point<std::chrono::system_clock> cycle_start;
};

#endif
