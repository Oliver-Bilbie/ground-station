#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <netinet/in.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Dispatcher {
 public:
  Dispatcher(int fd);
  ~Dispatcher();
  void set_target_address(sockaddr_in addr);
  void request_packet(uint64_t packet_number);
  /**
   * Mark the packet_number as received and request any previous packets that
   * have not yet arrived.
   */
  void receive(uint64_t packet_number);
  std::unordered_set<uint64_t> get_failed();
  bool is_ready;

 private:
  std::unordered_map<uint64_t, uint16_t> retry_counts;
  std::unordered_set<uint64_t> failed;
  uint64_t latest_packet_num = 0;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  int file_descriptor;
  sockaddr_in target_address;
};

#endif
