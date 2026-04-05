import Dashboard from "./Dashboard/Dashboard";
import { TelemetryProvider } from "./Telemetry";

function App() {
  return (
    <TelemetryProvider>
      <Dashboard />
    </TelemetryProvider>
  );
}

export default App;
