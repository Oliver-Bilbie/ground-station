import { useEffect, useRef } from "react";
import Globe from "globe.gl";
import { useTelemetry } from "../Telemetry";
import { cartesianToGlobe, idToColor, parsePacket } from "../helpers";
import EarthNight from "../assets/earth-night.jpg";
import NightSky from "../assets/night-sky.png";
import EarthTopology from "../assets/earth-topology.png";
import "./GlobeMap.css";

const GlobeMap = () => {
  const containerRef = useRef(null);
  const globeRef = useRef(null);
  const satData = useRef({});

  const { lastMessage } = useTelemetry();

  useEffect(() => {
    const width = containerRef.current.clientWidth;
    const height = containerRef.current.clientHeight;

    const globe = Globe()(containerRef.current)
      .width(width)
      .height(height)
      .globeImageUrl(EarthNight)
      .bumpImageUrl(EarthTopology)
      .backgroundImageUrl(NightSky)
      .particleLat("lat")
      .particleLng("lng")
      .particleAltitude("alt")
      .particlesList((d) => d.items)
      .particlesColor((d) => d.color)
      .particlesSize(5);

    globe.pointOfView({ altitude: 3.5 });

    globeRef.current = globe;

    const tick = () => {
      const satellites = Object.values(satData.current).filter(
        (d) => !isNaN(d.lat) && !isNaN(d.lng) && !isNaN(d.alt),
      );

      if (satellites.length === 0) {
        globeRef.current._tickReq = requestAnimationFrame(tick);
        return;
      }

      const groupedSets = Object.values(
        satellites.reduce((acc, sat) => {
          if (!acc[sat.color]) acc[sat.color] = { color: sat.color, items: [] };
          acc[sat.color].items.push(sat);
          return acc;
        }, {}),
      );

      globe.particlesData(groupedSets);
      globeRef.current._tickReq = requestAnimationFrame(tick);
    };

    tick();

    const handleResize = () => {
      if (containerRef.current) {
        const newWidth = containerRef.current.clientWidth;
        const newHeight = containerRef.current.clientHeight;
        globe.width(newWidth);
        globe.height(newHeight);
      }
    };

    window.addEventListener("resize", handleResize);

    return () => {
      window.removeEventListener("resize", handleResize);
      if (globeRef.current._tickReq)
        cancelAnimationFrame(globeRef.current._tickReq);
    };
  }, []);

  useEffect(() => {
    if (!lastMessage) return;

    try {
      const data = parsePacket(lastMessage);

      if (data.event === "disconnect") {
        delete satData.current[data.satellite_id];
        return;
      }

      if (data.event === "position") {
        const { satellite_id, position } = data;
        const { x, y, z } = position;

        const [lat, lng, alt] = cartesianToGlobe(x, y, z);

        let sat = satData.current[satellite_id];

        if (!sat) {
          sat = {
            id: satellite_id,
            color: idToColor(satellite_id),
          };
          satData.current[satellite_id] = sat;
        }

        // eslint-disable-next-line react-hooks/immutability
        sat.lat = lat;
        sat.lng = lng;
        sat.alt = alt;
        return;
      }
    } catch (err) {
      console.error("WS stream error:", err);
    }
  }, [lastMessage]);

  return <div ref={containerRef} className="globe-container" />;
};

export default GlobeMap;
