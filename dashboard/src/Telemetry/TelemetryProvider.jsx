import { useWebSocket } from "./useWebSocket";
import { TelemetryContext } from "./TelemetryContext";

export const TelemetryProvider = ({ children }) => {
  const socketValue = useWebSocket("ws://localhost:9001");

  return (
    <TelemetryContext.Provider value={socketValue}>
      {children}
    </TelemetryContext.Provider>
  );
};
