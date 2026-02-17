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
#include <random>
#include "buffer.h"
#include "client.h"
#include "globals.h"
#include "gps.h"
#include "packets.h"
#include "timer.h"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

uint64_t generate_id() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<uint64_t> dis;
  return dis(gen);
}

int main() {
  uint64_t satellite_id = generate_id();
  Client client(PORT, SERVER_IP);
  GPS satellite_gps;
  Timer timer;

  uint64_t packet_num = 1;
  Buffer<PositionPacket> buffer(100);
  bool packet_was_received = false;

  signal(SIGINT, signal_handler);

  while (running) {
    timer.reset();

    Position pos = satellite_gps.get_position();
    PositionPacket position_packet =
        PositionPacketData(satellite_id, packet_num, pos.timestamp, pos.x, pos.y, pos.z)
            .serialize();

    buffer.push(position_packet);
    client.send(position_packet);

    do {
      packet_was_received = false;
      int listen_time = CYCLE_LENGTH_MS - timer.elapsed();
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
