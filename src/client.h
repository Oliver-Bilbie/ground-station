#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include "space.h"

class Client {
 public:
  Client(int port, const char* server_ip) {
    file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (file_descriptor == -1) {
      std::cerr << "[ERROR] Error acquiring file descriptor" << std::endl;
      exit(1);
    }

    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_port = htons(port);
    if (inet_aton(server_ip, &_address.sin_addr) == 0) {
      std::cerr << "[ERROR] Invalid client address" << std::endl;
      close(file_descriptor);
      exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_aton(server_ip, &server_address.sin_addr) == 0) {
      std::cerr << "[ERROR] Invalid server address" << std::endl;
      close(file_descriptor);
      exit(1);
    }

    std::cout << "[INFO] Connected to UDP client at " << server_ip << ":" << port
              << std::endl;
  }

  const sockaddr_in address() { return _address; }

  template <typename T>
  void send(T packet) {
    space.send_message(packet, file_descriptor, server_address);
  }

  template <typename T>
  std::optional<T> listen(int timeout_ms) {
    struct pollfd pfd{file_descriptor, POLLIN};
    if (poll(&pfd, 1, timeout_ms) <= 0) {
      return std::nullopt;
    }

    T packet;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    int recv_len = recvfrom(file_descriptor,
                            &packet,
                            sizeof(packet),
                            0,
                            (struct sockaddr*)&server_addr,
                            &server_len);
    if (recv_len == 0) {
      return std::nullopt;
    }
    if (recv_len == -1) {
      std::cout << "[WARN] Received a malformed packet" << std::endl;
      return std::nullopt;
    }
    if (!(pfd.revents & POLLIN)) {
      std::cout << "[WARN] Received a malformed packet" << std::endl;
      return std::nullopt;
    }

    return packet;
  }

 private:
  int file_descriptor;
  struct sockaddr_in _address;
  struct sockaddr_in server_address;
  Space space;
};

#endif
