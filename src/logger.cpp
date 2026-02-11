#include "logger.h"
#include <iostream>

Logger::Logger() : last_printed(0) {};

void Logger::log(PositionPacketData packet) {
  buffer.push(packet);
  PositionPacketData next_packet = buffer.top();
  while (next_packet.packet_number <= last_printed + 1) {
    buffer.pop();
    if (next_packet.packet_number == last_printed + 1) {
      std::cout << next_packet.format();
      last_printed++;
    }
    if (buffer.empty()) {
      return;
    }
    next_packet = buffer.top();
  }
};
