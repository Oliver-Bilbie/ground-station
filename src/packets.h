#ifndef PACKETS_H
#define PACKETS_H

#include <cstdint>
#include <cstring>
#include <string>

#pragma pack(push, 1)
struct PositionPacket {
  uint64_t packet_number, timestamp, x, y, z;
};
#pragma pack(pop)

class PositionPacketData {
 public:
  uint64_t packet_number;
  uint64_t timestamp;
  double x, y, z;

  PositionPacketData(uint64_t packet_number,
                     uint64_t timestamp,
                     double x,
                     double y,
                     double z);
  PositionPacket serialize();
  static PositionPacketData deserialize(PositionPacket p);
  std::string format();
};

#pragma pack(push, 1)
struct NackPacket {
  uint64_t packet_number;
};
#pragma pack(pop)

class NackPacketData {
 public:
  uint64_t packet_number;

  NackPacketData(uint64_t packet_number);
  NackPacket serialize();
  static NackPacketData deserialize(NackPacket p);
};

#endif
