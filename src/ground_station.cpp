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
#include "packet.h"

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

  std::cout << "UDP Server listening on port " << PORT << " (Press Ctrl+C to stop)"
            << std::endl;

  int recv_len;
  Packet packet;
  struct pollfd pfd;
  pfd.fd = server_fd;
  pfd.events = POLLIN;

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

      PacketData packet_data = PacketData::deserialize(packet);
      std::cout << packet_data.format() << std::endl;
    }
  }

  std::cout << std::endl << "Cleaning up and exiting..." << std::endl;
  close(server_fd);
  return 0;
}
