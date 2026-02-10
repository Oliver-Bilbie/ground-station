#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_set>
#include "packets.h"
#include "simulate_noise.h"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

int main() {
  // Create socket file descriptor
  int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Configure server address
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // Bind socket to address
  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind");
    close(server_fd);
    exit(1);
  }

  std::cout << "[INFO] UDP Server listening on port " << PORT
            << " (Press Ctrl+C to stop)" << std::endl;

  int recv_len;
  PositionPacket packet;
  struct pollfd pfd;
  pfd.fd = server_fd;
  pfd.events = POLLIN;

  uint64_t latest_packet = 0;
  std::unordered_set<uint64_t> missing_packets;

  signal(SIGINT, signal_handler);

  while (running) {
    int poll_count = poll(&pfd, 1, 500);

    if (poll_count == -1) {
      // Handle user interruption
      if (errno == EINTR) {
        continue;
      }
      perror("poll");
      break;
    }

    if (poll_count == 0) {
      continue;
    }

    if (pfd.revents & POLLIN) {
      socklen_t client_len = sizeof(client_addr);
      recv_len = recvfrom(server_fd,
                          &packet,
                          sizeof(packet),
                          0,
                          (struct sockaddr*)&client_addr,
                          &client_len);

      if (recv_len == -1) {
        continue;
      }

      PositionPacketData position_data = PositionPacketData::deserialize(packet);
      std::cout << position_data.format() << std::endl;

      // Check for any previous packets that have not arrived
      for (int n = latest_packet + 1; n < position_data.packet_number; n++) {
        missing_packets.insert(n);
      }
      latest_packet = std::max(latest_packet, position_data.packet_number);

      // Check whether this packet was previously marked as missing
      missing_packets.erase(position_data.packet_number);

      // Request all missing packets
      for (const uint64_t& packet_num : missing_packets) {
        std::cout << "[INFO] Requesting missing packet: " << packet_num << std::endl;
        send_through_space(
            NackPacketData(packet_num).serialize(), server_fd, client_addr);
      }
    }
  }

  std::cout << std::endl << "[INFO] Cleaning up and exiting..." << std::endl;
  close(server_fd);
  return 0;
}
