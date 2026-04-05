#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <iostream>
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

  std::cout << std::endl
            << std::endl
            << "[INFO] " << dispatcher.get_failed().size() << " packets were lost"
            << std::endl;
  return 0;
}
