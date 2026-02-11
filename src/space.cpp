#include "space.h"
#include <random>

Space::Space() : is_running(true) {
  worker = std::thread([this]() {
    while (is_running) {
      Message msg;

      {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return (!message_queue.empty()) || !is_running; });

        if (!is_running) {
          return;
        }

        msg = message_queue.front();
        message_queue.pop();
      }

      if (!is_packet_loss()) {
        add_latency();
        sendto(msg.client_fd,
               msg.packet.data(),
               msg.packet.size(),
               0,
               (struct sockaddr*)&msg.server_addr,
               sizeof(msg.server_addr));
      }
    }
  });
}

Space::~Space() {
  is_running = false;
  cv.notify_all();
  if (worker.joinable()) {
    worker.join();
  }
}

int Space::get_random_int(int min, int max) {
  static thread_local std::random_device rd;
  static thread_local std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

bool Space::is_packet_loss() {
  // Drop 5% of packets
  return get_random_int(0, 19) == 0;
}

void Space::add_latency() {
  // Add a base latency
  int latency_ms = get_random_int(3, 5);

  // Introduce occasional spikes
  if (get_random_int(0, 9) == 0) {
    latency_ms += get_random_int(1, 200);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));
}
