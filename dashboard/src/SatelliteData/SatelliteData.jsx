import { useEffect, useState } from "react";
import { useTelemetry } from "../Telemetry";
import { cartesianToGeographic, idToColor } from "../helpers";
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
  const { satelliteData } = useTelemetry();
  const [localData, setLocalData] = useState({});
  const { dataRef } = satelliteData(false);

  useEffect(() => {
    const id = setInterval(() => {
      setLocalData({ ...dataRef.current });
    }, 100);
    return () => clearInterval(id);
  }, [dataRef]);

  return (
    <div className="satellite-container">
      {Object.entries(localData).map(([id, sat]) => (
        <div
          key={id}
          className="glass-panel card"
          style={{ borderLeftColor: idToColor(id) }}
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
