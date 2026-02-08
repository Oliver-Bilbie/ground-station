#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <cstring>
#include <string>

#pragma pack(push, 1)
struct Packet {
  uint64_t packet_number, timestamp, x, y, z;
};
#pragma pack(pop)

class PacketData {
 public:
  uint64_t packet_number;
  uint64_t timestamp;
  double x, y, z;

  PacketData(uint64_t packet_number, uint64_t timestamp, double x, double y, double z);
  Packet serialize();
  static PacketData deserialize(Packet p);
  std::string format();

 private:
  static uint64_t pack_double(double unpacked);
  static double unpack_double(uint64_t packed);
  static uint64_t htonll(uint64_t value);
  static uint64_t ntohll(uint64_t value);
};

#endif
