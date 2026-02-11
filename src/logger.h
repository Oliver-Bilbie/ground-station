#ifndef LOGGER_H
#define LOGGER_H

#include <queue>
#include "packets.h"

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
  uint64_t last_printed;
  std::priority_queue<PositionPacketData,
                      std::vector<PositionPacketData>,
                      MinHeapByPacketNum>
      buffer;
};

#endif
