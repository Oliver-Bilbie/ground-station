import { useEffect, useState } from "react";
import { useTelemetry } from "../Telemetry";
import { idToColor } from "../helpers";
import "./EventLog.css";

const MAX_EVENTS = 10;
const EVENT_TIMEOUT_MS = 5000;

const eventMsg = {
  dropped_packet: (e) =>
    `Packet dropped for Sat ${e.satellite_id}. Re-requesting...`,
  unavailable_packet: (e) => `CRITICAL: Packet lost for Sat ${e.satellite_id}.`,
  disconnect: (e) => `LINK LOST: Satellite ${e.satellite_id} offline.`,
  lost_packet_report: (e) =>
    `Satellite ${e.satellite_id} lost ${e.lost_packet_total} packets while connected.`,
};

const EventLog = () => {
  const [events, setEvents] = useState([]);
  const { satelliteEvents } = useTelemetry();

  useEffect(() => {
    const interval = setInterval(() => {
      const staleTime = Date.now() - EVENT_TIMEOUT_MS;
      const newEvents = satelliteEvents();
      setEvents((prev) =>
        [
          ...newEvents.map((e) => ({
            id: `${e.satellite_id}-${Date.now()}-${Math.random()}`,
            message: eventMsg[e.event](e),
            color: idToColor(e.satellite_id),
            timestamp: Date.now(),
            type: e.event,
          })),
          ...prev.filter((e) => e.timestamp > staleTime),
        ].slice(0, MAX_EVENTS),
      );
    }, 2000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="event-log-container">
      {events.map((item) => (
        <div
          key={item.id}
          className={`glass-panel card event-${item.type}`}
          style={{ borderLeftColor: item.color }}
        >
          <div className="event-content">
            <span className="timestamp">
              [{new Date(item.timestamp).toLocaleTimeString()}]
            </span>
            <h4>{item.message}</h4>
          </div>
        </div>
      ))}
    </div>
  );
};

export default EventLog;
