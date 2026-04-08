import { useEffect, useState } from "react";
import { useTelemetry } from "../Telemetry";
import { cartesianToGeographic, idToColor, parsePacket } from "../helpers";
import "./SatelliteData.css";

const formatPosition = (p) => {
  if (!p) return "unknown";
  const { x, y, z } = p;
  let [lon, lat, alt] = cartesianToGeographic(x, y, z);
  lon = lon.toFixed(2);
  lat = lat.toFixed(2);
  alt = (alt / 1000).toFixed(0);
  return `[${lon}°N, ${lat}°W, ${alt}km]`;
};

const SatelliteData = () => {
  const [satData, setSatData] = useState({});
  const { lastMessage } = useTelemetry();

  useEffect(() => {
    if (!lastMessage) return;

    try {
      const data = parsePacket(lastMessage);
      const { event, satellite_id } = data;

      setSatData((prevData) => {
        if (event === "disconnect") {
          const { [satellite_id]: _, ...rest } = prevData;
          return rest;
        }

        if (event === "position" || event === "latency") {
          const existingSat = prevData[satellite_id] || {
            color: idToColor(satellite_id),
          };

          return {
            ...prevData,
            [satellite_id]: {
              ...existingSat,
              ...(event === "position"
                ? { position: data.position }
                : { latency: data.latency }),
            },
          };
        }

        return prevData;
      });
    } catch (err) {
      console.error("Telemetry Processing Error:", err);
    }
  }, [lastMessage]);

  return (
    <div className="satellite-container">
      {Object.entries(satData).map(([id, sat]) => (
        <div
          key={id}
          className="glass-panel card"
          style={{ borderLeftColor: sat.color }}
        >
          <h3>
            Satellite <span className="telemetry">{id}</span>
          </h3>
          <div className="stats-row">
            <h4>
              Position:{" "}
              <span className="telemetry">{formatPosition(sat.position)}</span>
            </h4>
            <h4>
              Latency:{" "}
              <span className="telemetry">
                {sat.latency ? `${sat.latency}ms` : "---"}
              </span>
            </h4>
          </div>
        </div>
      ))}
    </div>
  );
};

export default SatelliteData;
