import { useEffect, useState } from "react";
import { useTelemetry } from "../Telemetry";
import { idToColor, parsePacket } from "../helpers";

const MAX_EVENTS = 8;
const EVENT_TIMEOUT_MS = 10000;

const EventLog = () => {
  const [events, setEvents] = useState([]);
  const { lastMessage } = useTelemetry();

  useEffect(() => {
    const interval = setInterval(() => {
      const staleTime = Date.now() - EVENT_TIMEOUT_MS;
      setEvents((prev) => prev.filter((e) => e.timestamp > staleTime));
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    if (!lastMessage) return;

    try {
      const data = parsePacket(lastMessage);
      const { event, satellite_id } = data;

      const eventMap = {
        dropped_packet: `Packet dropped for Sat ${satellite_id}. Re-requesting...`,
        unavailable_packet: `CRITICAL: Packet lost for Sat ${satellite_id}.`,
        disconnect: `LINK LOST: Satellite ${satellite_id} offline.`,
      };

      if (eventMap[event]) {
        const newEvent = {
          id: `${satellite_id}-${Date.now()}-${Math.random()}`,
          message: eventMap[event],
          color: idToColor(satellite_id),
          timestamp: Date.now(),
          type: event,
        };

        //eslint-disable-next-line react-hooks/set-state-in-effect
        setEvents((prev) => [newEvent, ...prev].slice(0, MAX_EVENTS));
      }
    } catch (err) {
      console.error("Event Log Error:", err);
    }
  }, [lastMessage]);

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
            <h4 className="telemetry">{item.message}</h4>
          </div>
        </div>
      ))}
    </div>
  );
};

export default EventLog;
