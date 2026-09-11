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
#include <string>
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

int main(int argc, char* argv[]) {
  bool write_stdout = false;
  bool write_ws = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--stdout") {
      write_stdout = true;
    } else if (arg == "--ws") {
      write_ws = true;
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl
                << "Usage: GroundStation [--stdout] [--ws]" << std::endl;
      return 1;
    }
  }
  if (!write_stdout && !write_ws) {
    write_stdout = true;
  }

  auto server = std::make_shared<Server>(PORT, write_stdout);
  auto telemetry = std::make_shared<TelemetryServer>(TELEMETRY_PORT);
  Dispatcher dispatcher(server, telemetry, write_stdout);
  Logger logger(telemetry, write_stdout, write_ws);

  LatencyTracker latencies(telemetry, write_stdout);
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
