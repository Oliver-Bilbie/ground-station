#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <optional>
#include "buffer.h"
#include "client.h"
#include "globals.h"
#include "gps.h"
#include "packets.h"

#define SERVER_IP "127.0.0.1"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

int time_since_start(
    std::chrono::time_point<std::chrono::high_resolution_clock> cycle_start) {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::high_resolution_clock::now() - cycle_start)
      .count();
}

int main() {
  Client client(PORT, SERVER_IP);
  GPS satellite_gps;

  uint64_t packet_num = 1;
  Buffer<PositionPacket> buffer(100);
  bool packet_was_received = false;

  signal(SIGINT, signal_handler);

  while (running) {
    auto cycle_start = std::chrono::high_resolution_clock::now();

    Position pos = satellite_gps.get_position();
    PositionPacket position_packet =
        PositionPacketData(packet_num, pos.timestamp, pos.x, pos.y, pos.z).serialize();

    buffer.push(position_packet);
    client.send(position_packet);

    do {
      packet_was_received = false;
      int listen_time = CYCLE_LENGTH_MS - time_since_start(cycle_start);
      if (listen_time > 0) {
        auto result = client.listen<NackPacket>(listen_time);
        if (result.has_value()) {
          packet_was_received = true;
          NackPacketData nack_data = NackPacketData::deserialize(result.value());
          auto buffer_result = buffer.get(packet_num - nack_data.packet_number);
          if (buffer_result.has_value()) {
            client.send(buffer_result.value());
            std::cout << "[INFO] Re-sent packet " << nack_data.packet_number
                      << std::endl;
          } else {
            std::cout << "[WARN] Packet " << nack_data.packet_number
                      << " was re-requested, but is no longer available" << std::endl;
          }
        }
      }
    } while (packet_was_received);

    packet_num++;
  }

  std::cout << std::endl << "[INFO] Cleaning up and exiting..." << std::endl;
  return 0;
}
