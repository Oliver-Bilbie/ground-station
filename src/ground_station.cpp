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
#include "dispatcher.h"
#include "logger.h"
#include "packets.h"

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
  int gs_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (gs_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Configure server address
  struct sockaddr_in gs_addr, satellite_addr;
  socklen_t satellite_len = sizeof(satellite_addr);
  memset(&gs_addr, 0, sizeof(gs_addr));
  gs_addr.sin_family = AF_INET;
  gs_addr.sin_addr.s_addr = INADDR_ANY;
  gs_addr.sin_port = htons(PORT);

  // Bind socket to address
  if (bind(gs_fd, (struct sockaddr*)&gs_addr, sizeof(gs_addr)) == -1) {
    perror("bind");
    close(gs_fd);
    exit(1);
  }

  std::cout << "[INFO] UDP Server listening on port " << PORT
            << " (Press Ctrl+C to stop)" << std::endl;

  int recv_len;
  PositionPacket packet;
  struct pollfd pfd;
  pfd.fd = gs_fd;
  pfd.events = POLLIN;

  Logger logger;
  uint64_t latest_packet = 0;
  Dispatcher dispatcher(32, gs_fd);

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
      socklen_t satellite_len = sizeof(satellite_addr);
      recv_len = recvfrom(gs_fd,
                          &packet,
                          sizeof(packet),
                          0,
                          (struct sockaddr*)&satellite_addr,
                          &satellite_len);
      if (recv_len == -1) {
        continue;
      }
      if (!dispatcher.is_ready) {
        dispatcher.set_target_address(satellite_addr);
      }

      PositionPacketData position_data = PositionPacketData::deserialize(packet);
      logger.log(position_data);

      // Check for any previous packets that have not arrived
      for (int n = latest_packet + 1; n < position_data.packet_number; n++) {
        dispatcher.request_packet(n);
      }
      latest_packet = std::max(latest_packet, position_data.packet_number);

      // Check whether this packet was previously marked as missing
      dispatcher.mark_received(position_data.packet_number);
    }
  }

  std::cout << std::endl
            << std::endl
            << "[INFO] " << dispatcher.get_failed().size() << " packets were lost"
            << std::endl;
  close(gs_fd);
  return 0;
}
