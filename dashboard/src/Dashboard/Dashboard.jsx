import EventLog from "../EventLog/EventLog";
import GlobeMap from "../GlobeMap/GlobeMap";
import SatelliteData from "../SatelliteData/SatelliteData";
import { useTelemetry } from "../Telemetry";
import "./Dashboard.css";

function Dashboard() {
  const { status } = useTelemetry();

  return (
    <div className="mission-control">
      <div className="globe-layer">
        <GlobeMap />
      </div>

      <div className="overlay-container">
        <header className="top-bar">
          <h1>SATELLITE MISSION CONTROL</h1>
          <div className="system-status">LINK: {status}</div>
        </header>

        {status === "Open" && (
          <aside className="panel left-panel">
            <SatelliteData />
          </aside>
        )}

        <aside className="panel right-panel">
          <EventLog />
        </aside>

        <footer className="bottom-bar">
          <a
            href="https://github.com/Oliver-Bilbie/ground-station"
            target="_blank"
          >
            <h5>READ MORE ABOUT THIS PROJECT</h5>
          </a>
        </footer>
      </div>
    </div>
  );
}

export default Dashboard;
