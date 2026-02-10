#ifndef NOISE_H
#define NOISE_H

#include <netinet/in.h>
#include "packets.h"

void send_through_space(PositionPacket packet, int client_fd, sockaddr_in server_addr);

#endif
