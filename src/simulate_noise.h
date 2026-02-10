#ifndef NOISE_H
#define NOISE_H

#include <netinet/in.h>
#include "packet.h"

void send_through_space(Packet packet, int client_fd, sockaddr_in server_addr);

#endif
