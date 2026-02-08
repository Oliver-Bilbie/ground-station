#include "packet.h"
#include <netinet/in.h>
#include <chrono>
#include <iomanip>
#include <sstream>

PacketData::PacketData(uint64_t pn, uint64_t ts, double _x, double _y, double _z)
    : packet_number(pn), timestamp(ts), x(_x), y(_y), z(_z) {}

Packet PacketData::serialize() {
  return Packet{htonll(packet_number),
                htonll(timestamp),
                htonll(pack_double(x)),
                htonll(pack_double(y)),
                htonll(pack_double(z))};
}

PacketData PacketData::deserialize(Packet p) {
  return PacketData(ntohll(p.packet_number),
                    ntohll(p.timestamp),
                    unpack_double(ntohll(p.x)),
                    unpack_double(ntohll(p.y)),
                    unpack_double(ntohll(p.z)));
}

std::string PacketData::format() {
  auto ms = std::chrono::milliseconds(timestamp);
  auto time_point = std::chrono::system_clock::from_time_t(
      static_cast<std::time_t>(ms.count() / 1000));
  std::time_t t = std::chrono::system_clock::to_time_t(time_point);
  std::tm tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] Satellite is located at ("
      << x << ", " << y << ", " << z << ")" << std::endl;
  return oss.str();
}

uint64_t PacketData::pack_double(double unpacked) {
  uint64_t packed;
  std::memcpy(&packed, &unpacked, sizeof(packed));
  return packed;
}

double PacketData::unpack_double(uint64_t packed) {
  double unpacked;
  std::memcpy(&unpacked, &packed, sizeof(unpacked));
  return unpacked;
}

uint64_t PacketData::htonll(uint64_t value) {
  int num = 42;
  if (*(char*)&num == 42) {  // Check if we are on a Little Endian machine
    uint32_t high_part = htonl((uint32_t)(value >> 32));
    uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
    return ((uint64_t)low_part << 32) | high_part;
  }
  return value;  // We are already Big Endian, do nothing
}

uint64_t PacketData::ntohll(uint64_t value) {
  return htonll(value);  // The swap is symmetric
}
