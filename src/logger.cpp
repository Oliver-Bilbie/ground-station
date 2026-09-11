#include "logger.h"
#include <iostream>
#include <memory>
#include <sstream>
#include "globals.h"

Logger::Logger(std::shared_ptr<TelemetryServer> telemetry_server,
               bool write_stdout,
               bool write_ws)
    : telemetry(telemetry_server), write_stdout(write_stdout), write_ws(write_ws) {
  if (write_ws && telemetry_server == nullptr) {
    std::cout << "[WARN] Logger was initialized without a telemetry server"
              << std::endl;
  }
};

void Logger::output(const std::string& text, const std::string& json) {
  if (write_stdout) {
    std::cout << text;
  }
  if (write_ws && telemetry != nullptr) {
    telemetry->publish(json);
  }
}

void Logger::log(PositionPacketData packet) {
  LoggerState& state = satellite_states[packet.satellite_id];
  state.buffer.push(packet);
  process_buffer(packet.satellite_id);

  while (state.buffer.size() >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
    std::ostringstream text_oss;
    text_oss << "[WARN] Packet " << state.last_output + 1 << " from satellite "
             << packet.satellite_id << " was not received" << std::endl;
    std::ostringstream json_oss;
    json_oss << "{\"event\": \"unavailable_packet\", \"satellite_id\": \""
             << packet.satellite_id << "\", \"packet_number\": \""
             << state.last_output + 1 << "\"}" << std::endl;
    output(text_oss.str(), json_oss.str());

    state.last_output++;
    process_buffer(packet.satellite_id);
  }
};

void Logger::on_disconnect(uint64_t satellite_id) {
  satellite_states.erase(satellite_id);
}

void Logger::process_buffer(uint64_t satellite_id) {
  LoggerState& state = satellite_states[satellite_id];

  if (state.buffer.empty()) {
    return;
  }
  PositionPacketData next_packet = state.buffer.top();

  while (next_packet.packet_number <= state.last_output + 1) {
    state.buffer.pop();

    if (next_packet.packet_number == state.last_output + 1) {
      output(next_packet.text(), next_packet.json());
      state.last_output++;
    }

    if (state.buffer.empty()) {
      return;
    }
    next_packet = state.buffer.top();
  }
}
