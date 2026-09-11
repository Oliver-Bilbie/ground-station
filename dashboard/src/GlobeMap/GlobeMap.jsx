import { useEffect, useRef } from "react";
import Globe from "globe.gl";
import { useTelemetry } from "../Telemetry";
import { cartesianToGlobe, idToColor } from "../helpers";
import EarthNight from "../assets/earth-night.jpg";
import NightSky from "../assets/night-sky.png";
import EarthTopology from "../assets/earth-topology.png";
import "./GlobeMap.css";

const GlobeMap = () => {
  const containerRef = useRef(null);
  const globeRef = useRef(null);
  const { satelliteData } = useTelemetry();

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
      globeRef.current._tickReq = requestAnimationFrame(tick);
      const { dataRef, isNew } = satelliteData(true);
      if (!isNew) return;
      globe.particlesData(
        Object.entries(dataRef.current)
          .filter(
            ([_, sat]) =>
              !isNaN(sat.position?.x) &&
              !isNaN(sat.position?.y) &&
              !isNaN(sat.position?.z),
          )
          .map(([id, sat]) => ({
            color: idToColor(id),
            items: [
              cartesianToGlobe(sat.position.x, sat.position.y, sat.position.z),
            ],
          })),
      );
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

  return <div ref={containerRef} className="globe-container" />;
};

export default GlobeMap;
