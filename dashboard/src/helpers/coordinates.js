const EARTH_RADIUS_METERS = 6371000;

export const cartesianToGlobe = (x, y, z) => {
  let [lat, lng, alt] = cartesianToGeographic(x, y, z);
  alt = alt / EARTH_RADIUS_METERS;
  return [lat, lng, alt];
};

export const cartesianToGeographic = (x, y, z) => {
  const lat = Math.atan2(z, Math.sqrt(x * x + y * y)) * (180 / Math.PI);
  const lng = Math.atan2(y, x) * (180 / Math.PI);
  const alt = Math.sqrt(x * x + y * y + z * z) - EARTH_RADIUS_METERS;
  return [lat, lng, alt];
};
