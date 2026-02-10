#include "simulate_noise.h"
#include <random>

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
