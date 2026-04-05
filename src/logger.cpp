#include "logger.h"
#include <iostream>
#include <memory>
#include <sstream>
#include "globals.h"

Logger::Logger(std::shared_ptr<TelemetryServer> telemetry_server)
    : telemetry(telemetry_server), last_output(0) {
  if (telemetry_server == nullptr) {
    std::cout << "[WARN] Logger was initialized without a telemetry server"
              << std::endl;
  }
};

void Logger::log(PositionPacketData packet) {
  buffers[packet.satellite_id].push(packet);
  process_buffer(packet.satellite_id);

  while (buffers[packet.satellite_id].size() >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
    std::cout << "[WARN] Packet " << last_output[packet.satellite_id] + 1
              << " from satellite " << packet.satellite_id << " was not received"
              << std::endl;

    if (telemetry != nullptr) {
      std::ostringstream json_oss;
      json_oss << "{\"event\": \"unavailable_packet\", \"satellite_id\": "
               << packet.satellite_id << "}" << std::endl;
      telemetry->publish(json_oss.str());
    }

    last_output[packet.satellite_id]++;
    process_buffer(packet.satellite_id);
  }
};

void Logger::process_buffer(uint64_t satellite_id) {
  auto& buffer = buffers[satellite_id];
  if (buffer.empty()) {
    return;
  }
  PositionPacketData next_packet = buffer.top();

  while (next_packet.packet_number <= last_output[satellite_id] + 1) {
    buffer.pop();

    if (next_packet.packet_number == last_output[satellite_id] + 1) {
      std::cout << next_packet.text();
      if (telemetry != nullptr) {
        telemetry->publish(next_packet.json());
      }
      last_output[satellite_id]++;
    }

    if (buffer.empty()) {
      return;
    }
    next_packet = buffer.top();
  }
}
