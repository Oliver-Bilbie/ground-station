#ifndef SPACE_H
#define SPACE_H

#include <netinet/in.h>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>

struct Message {
  // store raw bytes so that any type of packet can be used with the same Space instance
  std::vector<uint8_t> packet;
  int client_fd;
  sockaddr_in server_addr;
};

/**
 * Send packets with added noise to simulate a signal travelling through space.
 * Packets will randomly be delayed or lost.
 */
class Space {
 public:
  Space();
  ~Space();

  template <typename T>
  void send_message(const T& packet, int client_fd, sockaddr_in server_addr) {
    const uint8_t* raw_ptr = reinterpret_cast<const uint8_t*>(&packet);
    std::vector<uint8_t> data(raw_ptr, raw_ptr + sizeof(T));

    std::lock_guard<std::mutex> lock(mtx);
    message_queue.push(Message{std::move(data), client_fd, server_addr});
    cv.notify_all();
  };

 private:
  std::atomic<bool> is_running;
  std::queue<Message> message_queue;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  int get_random_int(int min, int max);
  bool is_packet_loss();
  void add_latency();
};

#endif
