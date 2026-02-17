#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include "space.h"

template <typename T>
struct ListenResponse {
  T packet;
  struct sockaddr_in client;
};

class Server {
 public:
  Server(int port) {
    file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (file_descriptor == -1) {
      std::cerr << "[ERROR] Error acquiring file descriptor" << std::endl;
      exit(1);
    }

    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(port);

    if (bind(file_descriptor, (struct sockaddr*)&_address, sizeof(_address)) == -1) {
      std::cerr << "[ERROR] Unable to bind server to port " << port << std::endl;
      close(file_descriptor);
      exit(1);
    }

    std::cout << "[INFO] UDP Server listening on port " << port << std::endl;
  }

  const sockaddr_in address() { return _address; }

  template <typename T>
  void send(T packet, sockaddr_in target_address) {
    space.send_message(packet, file_descriptor, target_address);
  }

  template <typename T>
  std::optional<ListenResponse<T>> listen(int timeout_ms) {
    struct pollfd pfd{file_descriptor, POLLIN};
    if (poll(&pfd, 1, timeout_ms) <= 0) {
      return std::nullopt;
    }

    T packet;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int recv_len = recvfrom(file_descriptor,
                            &packet,
                            sizeof(packet),
                            0,
                            (struct sockaddr*)&client_addr,
                            &client_len);
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

    return ListenResponse<T>{packet, client_addr};
  }

 private:
  int file_descriptor;
  struct sockaddr_in _address;
  Space space;
};

#endif
