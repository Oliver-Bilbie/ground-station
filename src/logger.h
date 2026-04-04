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

class Logger {
 public:
  Logger();
  void log(PositionPacketData packet);

 private:
  std::unordered_map<uint64_t, uint64_t> last_output;
  std::unordered_map<uint64_t,
                     std::priority_queue<PositionPacketData,
                                         std::vector<PositionPacketData>,
                                         MinHeapByPacketNum>>
      buffers;

  std::unique_ptr<TelemetryServer> telemetry;

  void process_buffer(uint64_t satellite_id);

  friend class LoggerTest;
};

#endif
