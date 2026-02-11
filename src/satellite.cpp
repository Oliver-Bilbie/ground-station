#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include "buffer.h"
#include "globals.h"
#include "gps.h"
#include "packets.h"
#include "simulate_noise.h"

#define SERVER_IP "127.0.0.1"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

int main() {
  // Create UDP socket
  int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_fd == -1) {
    perror("socket");
    exit(1);
  }

  struct pollfd pfd;
  pfd.fd = client_fd;
  pfd.events = POLLIN;

  // Configure server address
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0) {
    std::cerr << "[ERROR] Invalid server address\n";
    close(client_fd);
    exit(1);
  }

  std::cout << "[INFO] Connected to UDP server at " << SERVER_IP << ":" << PORT
            << " (Press Ctrl+C to stop)" << std::endl;

  GPS satellite_gps;
  uint64_t packet_num = 1;
  Buffer<PositionPacket> buffer(100);

  signal(SIGINT, signal_handler);

  while (running) {
    auto cycle_start = std::chrono::high_resolution_clock::now();

    Position pos = satellite_gps.get_position();
    PositionPacket position_packet =
        PositionPacketData(packet_num, pos.timestamp, pos.x, pos.y, pos.z).serialize();

    buffer.push(position_packet);
    send_through_space(position_packet, client_fd, server_addr);

    int poll_count = poll(&pfd, 1, CYCLE_LENGTH_MS - 20);
    NackPacket nack_packet;
    if (pfd.revents & POLLIN) {
      socklen_t client_len = sizeof(server_addr);
      int recv_len = recvfrom(client_fd,
                              &nack_packet,
                              sizeof(nack_packet),
                              0,
                              (struct sockaddr*)&server_addr,
                              &client_len);

      if (recv_len != -1) {
        NackPacketData nack_data = NackPacketData::deserialize(nack_packet);
        std::optional<PositionPacket> buffer_result =
            buffer.get(packet_num - nack_data.packet_number);
        if (buffer_result.has_value()) {
          send_through_space(buffer_result.value(), client_fd, server_addr);
          std::cout << "[INFO] Re-sent packet " << nack_data.packet_number << std::endl;
        } else {
          std::cout << "[WARN] Packet " << nack_data.packet_number
                    << " was re-requested, but is no longer available" << std::endl;
        }
      }
    }

    int cycle_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::high_resolution_clock::now() - cycle_start)
                             .count();
    if (cycle_duration < CYCLE_LENGTH_MS) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(CYCLE_LENGTH_MS - cycle_duration));
    }

    packet_num++;
  }

  std::cout << std::endl << "[INFO] Cleaning up and exiting..." << std::endl;
  close(client_fd);
  return 0;
}
