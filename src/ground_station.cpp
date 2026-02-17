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
#include "timer.h"

std::atomic<bool> running(true);
void signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
  }
}

int main() {
  std::shared_ptr<Server> server = std::make_shared<Server>(PORT);
  Logger logger;
  RollingAverage<uint64_t> latency(100);
  Dispatcher dispatcher(server);
  Timer timer;

  signal(SIGINT, signal_handler);

  while (running) {
    auto response = server->listen<PositionPacket>(500);
    if (response.has_value()) {
      auto position_data = PositionPacketData::deserialize(response.value().packet);
      dispatcher.receive(position_data.satellite_id,
                         position_data.packet_number,
                         response.value().client);
      logger.log(position_data);
      timer.set(position_data.timestamp);
      latency.add_contribution(timer.elapsed());
    }
  }

  std::cout << std::endl
            << std::endl
            << "[INFO] " << dispatcher.get_failed().size() << " packets were lost"
            << std::endl
            << "[INFO] Average latency: " << latency.get_value() << "ms" << std::endl;
  return 0;
}
