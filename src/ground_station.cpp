#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include "dispatcher.h"
#include "globals.h"
#include "logger.h"
#include "packets.h"
#include "rolling_avg.h"
#include "server.h"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

uint64_t ms_since_timestamp(uint64_t timestamp) {
  uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  if (timestamp > now) {
    return 0;
  }
  return now - timestamp;
}

int main() {
  std::shared_ptr<Server> server = std::make_shared<Server>(PORT);
  Logger logger;
  RollingAverage<uint64_t> latency(100);
  Dispatcher dispatcher(server);

  signal(SIGINT, signal_handler);

  while (running) {
    auto response = server->listen<PositionPacket>(500);
    if (response.has_value()) {
      PositionPacketData position_data =
          PositionPacketData::deserialize(response.value().packet);
      dispatcher.receive(position_data.packet_number, response.value().client);
      logger.log(position_data);
      latency.add_contribution(ms_since_timestamp(position_data.timestamp));
    }
  }

  std::cout << std::endl
            << std::endl
            << "[INFO] " << dispatcher.get_failed().size() << " packets were lost"
            << std::endl
            << "[INFO] Average latency: " << latency.get_value() << "ms" << std::endl;
  return 0;
}
