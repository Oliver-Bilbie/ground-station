#ifndef NOISE_H
#define NOISE_H

#include <netinet/in.h>
#include <thread>

int get_random_int(int min, int max);

bool is_packet_loss();

void add_latency();

template <typename T>
void send_through_space(T packet, int client_fd, sockaddr_in server_addr) {
  // This part of the process shouldn't impact the main satellite functionality, so
  // we are going to run it in a separate thread in a 'fire and forget' fashion.
  //
  // Spawning a new thread for each packet is far too expensive for a high throughput
  // system but good enough for now. I may introduce a thread pool later on.
  std::thread t(
      [](T packet, int client_fd, sockaddr_in server_addr) {
        if (is_packet_loss()) {
          return;
        }

        add_latency();

        sendto(client_fd,
               &packet,
               sizeof(packet),
               0,
               (struct sockaddr*)&server_addr,
               sizeof(server_addr));
      },
      packet,
      client_fd,
      server_addr);
  t.detach();
}

#endif
