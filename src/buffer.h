#ifndef BUFFER_H
#define BUFFER_H

#include <deque>
#include <optional>

template <typename T>
class Buffer {
 private:
  int buffer_size;
  std::deque<T> items;

 public:
  Buffer(int cs) : buffer_size(cs) {};

  /**
   * Add an item to the buffer.
   * If the buffer exceeds the size limit, previous items will be removed.
   */
  void push(T value) {
    items.push_back(value);
    while (items.size() >= buffer_size) {
      items.pop_front();
    }
  };

  /**
   * Get the buffer item from n entries ago.
   * n = 0 will return the most recent item.
   */
  std::optional<T> get(int n) {
    if (n >= items.size() || n < 0) {
      return std::nullopt;
    }
    return items[items.size() - 1 - n];
  };
};

#endif
