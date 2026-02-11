#ifndef ROLLING_AVG_H
#define ROLLING_AVG_H

#include <queue>

template <typename T>
class RollingAverage {
 public:
  RollingAverage(int len) : max_length(len), current_sum(0.0) {}

  double get_value() {
    if (values.empty())
      return 0.0;
    return current_sum / values.size();
  }

  void add_contribution(T val) {
    values.push(val);
    current_sum += val;

    if (values.size() > max_length) {
      T removed = values.front();
      values.pop();
      current_sum -= removed;
    }
  }

 private:
  int max_length;
  double current_sum;
  std::queue<T> values;
};

#endif
