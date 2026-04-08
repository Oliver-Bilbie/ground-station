export const idToColor = (id) => {
  const idStr = id.toString();
  let hash = 0;
  for (let i = 0; i < idStr.length; i++) {
    // Bitwise shift to scramble the bits
    hash = idStr.charCodeAt(i) + ((hash << 5) - hash);
  }

  // Use the Golden Ratio to spread the hue
  const phi = 0.618033988749895;
  let h = Math.abs(hash) * phi;
  h -= Math.floor(h);

  const hue = Math.floor(h * 360);

  return `hsl(${hue},70%,60%)`;
};
