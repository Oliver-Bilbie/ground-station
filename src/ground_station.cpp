#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <memory>
#include <optional>
#include "dispatcher.h"
#include "globals.h"
#include "latency_tracker.h"
#include "logger.h"
#include "packets.h"
#include "server.h"
#include "telemetry_server.h"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

int main() {
  auto server = std::make_shared<Server>(PORT);
  auto telemetry = std::make_shared<TelemetryServer>(TELEMETRY_PORT);
  Dispatcher dispatcher(server, telemetry);
  Logger logger(telemetry);

  LatencyTracker latencies(telemetry);
  latencies.on_disconnect([&dispatcher](uint64_t id) { dispatcher.on_disconnect(id); });
  latencies.on_disconnect([&logger](uint64_t id) { logger.on_disconnect(id); });

  signal(SIGINT, signal_handler);

  while (running) {
    auto response = server->listen<PositionPacket>(500);
    if (response.has_value()) {
      auto position_data = PositionPacketData::deserialize(response.value().packet);
      dispatcher.receive(position_data.satellite_id,
                         position_data.packet_number,
                         response.value().client);
      logger.log(position_data);
      latencies.add_contribution(position_data.satellite_id, position_data.timestamp);
    }
  }

  return 0;
}
