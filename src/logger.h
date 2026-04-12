#ifndef LOGGER_H
#define LOGGER_H

#include <cstdint>
#include <memory>
#include <queue>
#include <unordered_map>
#include "packets.h"
#include "telemetry_server.h"

struct MinHeapByPacketNum {
  bool operator()(const PositionPacketData& a, const PositionPacketData& b) {
    return a.packet_number > b.packet_number;
  }
};

struct LoggerState {
  uint64_t last_output;
  std::priority_queue<PositionPacketData,
                      std::vector<PositionPacketData>,
                      MinHeapByPacketNum>
      buffer;
};

class Logger {
 public:
  Logger(std::shared_ptr<TelemetryServer> telemetry_server);
  void log(PositionPacketData packet);
  void on_disconnect(uint64_t satellite_id);

 private:
  std::unordered_map<uint64_t, LoggerState> satellite_states;

  std::shared_ptr<TelemetryServer> telemetry;

  void process_buffer(uint64_t satellite_id);

  friend class LoggerTest;
};

#endif
