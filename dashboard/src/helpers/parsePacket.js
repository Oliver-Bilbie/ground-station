export const parsePacket = (packet) => {
  return JSON.parse(packet, (key, value) => {
    if (key === "satellite_id") return BigInt(value).toString(36).toUpperCase();
    return value;
  });
};
