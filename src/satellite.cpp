#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include "gps.h"
#include "packet.h"
#include "simulate_noise.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 512

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

  // Configure server address
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0) {
    std::cerr << "Invalid server address\n";
    close(client_fd);
    exit(1);
  }

  std::cout << "Connected to UDP server at " << SERVER_IP << ":" << PORT
            << " (Press Ctrl+C to stop)" << std::endl;

  GPS satellite_gps;
  uint64_t packet_num = 0;
  signal(SIGINT, signal_handler);

  while (running) {
    Position pos = satellite_gps.get_position();
    Packet packet =
        PacketData(packet_num, pos.timestamp, pos.x, pos.y, pos.z).serialize();
    send_through_space(packet, client_fd, server_addr);
    packet_num++;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << std::endl << "Cleaning up and exiting..." << std::endl;
  close(client_fd);
  return 0;
}
