#include "logger.h"
#include <iostream>
#include "globals.h"

Logger::Logger() : last_printed(0) {};

void Logger::log(PositionPacketData packet) {
  buffer.push(packet);
  process_buffer();

  while (buffer.size() >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
    std::cout << "[WARN] Packet " << last_printed + 1 << " was not received"
              << std::endl;
    last_printed++;
    process_buffer();
  }
};

void Logger::process_buffer() {
  if (buffer.empty()) {
    return;
  }
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
}
