#include "simulate_noise.h"
#include <random>
#include <thread>

int get_random_int(int min, int max) {
  static thread_local std::random_device rd;
  static thread_local std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

bool is_packet_loss() {
  // Drop 5% of packets
  return get_random_int(0, 19) == 0;
}

void add_latency() {
  // Add a base latency
  int latency_ms = get_random_int(3, 5);

  // Introduce occasional spikes
  if (get_random_int(0, 9) == 0) {
    latency_ms += get_random_int(1, 200);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));
}

void _send_through_space(PositionPacket packet,
                         int client_fd,
                         sockaddr_in server_addr) {
  if (is_packet_loss()) {
    return;
  }

  add_latency();

  sendto(client_fd,
         &packet,
         sizeof(packet),
         0,
         (struct sockaddr*)&server_addr,
         sizeof(server_addr));
}

void send_through_space(PositionPacket packet, int client_fd, sockaddr_in server_addr) {
  // This part of the process shouldn't impact the main satellite functionality, so
  // we are going to run it in a separate thread in a 'fire and forget' fashion.
  //
  // Spawning a new thread for each packet is far too expensive for a high throughput
  // system but good enough for now. I may introduce a thread pool later on.
  std::thread t(_send_through_space, packet, client_fd, server_addr);
  t.detach();
}
