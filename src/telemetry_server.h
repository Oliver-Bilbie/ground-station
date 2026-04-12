#ifndef TELEMETRY_SERVER_H
#define TELEMETRY_SERVER_H

#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include "App.h"

struct PerSocketData {};

class TelemetryServer {
 private:
  std::thread worker;
  std::unique_ptr<uWS::App> app;
  uWS::Loop* loop = nullptr;
  us_listen_socket_t* listen_socket = nullptr;
  std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;

 public:
  TelemetryServer(int port);
  ~TelemetryServer();
  // Removing these methods prevents accidental copying which would cause
  // two threads trying to manage the same pointers.
  TelemetryServer(const TelemetryServer&) = delete;
  TelemetryServer& operator=(const TelemetryServer&) = delete;

  void publish(std::string payload);
};

#endif
