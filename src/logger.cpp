#include "logger.h"
#include <iostream>
#include "globals.h"

Logger::Logger() : last_printed(0) {};

void Logger::log(PositionPacketData packet) {
  buffers[packet.satellite_id].push(packet);
  process_buffer(packet.satellite_id);

  while (buffers[packet.satellite_id].size() >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
    std::cout << "[WARN] Packet " << last_printed[packet.satellite_id] + 1
              << " from satellite " << packet.satellite_id << " was not received"
              << std::endl;
    last_printed[packet.satellite_id]++;
    process_buffer(packet.satellite_id);
  }
};

void Logger::process_buffer(uint64_t satellite_id) {
  auto& buffer = buffers[satellite_id];
  if (buffer.empty()) {
    return;
  }
  PositionPacketData next_packet = buffer.top();

  while (next_packet.packet_number <= last_printed[satellite_id] + 1) {
    buffer.pop();

    if (next_packet.packet_number == last_printed[satellite_id] + 1) {
      std::cout << next_packet.format();
      last_printed[satellite_id]++;
    }

    if (buffer.empty()) {
      return;
    }
    next_packet = buffer.top();
  }
}
