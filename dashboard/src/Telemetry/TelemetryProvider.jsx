import { useWebSocket } from "./useWebSocket";
import { TelemetryContext } from "./TelemetryContext";

export const TelemetryProvider = ({ children }) => {
  const socketValue = useWebSocket();

  return (
    <TelemetryContext.Provider value={socketValue}>
      {children}
    </TelemetryContext.Provider>
  );
};
