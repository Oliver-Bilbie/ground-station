#ifndef TELEMETRY_SERVER_H
#define TELEMETRY_SERVER_H

#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
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
  TelemetryServer(int port) {
    std::promise<bool> init_promise;
    auto init_future = init_promise.get_future();

    worker = std::thread([this, port, &init_promise]() {
      this->app = std::make_unique<uWS::App>();
      this->loop = uWS::Loop::get();

      app->ws<PerSocketData>("/*",
                             {.compression = uWS::SHARED_COMPRESSOR,
                              .idleTimeout = 60,
                              .open =
                                  [this](auto* ws) {
                                    this->clients.insert(ws);
                                    ws->subscribe("telemetry_stream");
                                  },
                              .close =
                                  [this](auto* ws, int code, std::string_view message) {
                                    this->clients.erase(ws);
                                  }})
          .listen(port, [this, &init_promise](auto* token) {
            this->listen_socket = token;
            init_promise.set_value(token != nullptr);
          });

      if (this->listen_socket) {
        app->run();
      }

      this->app = nullptr;
      this->loop = nullptr;
      this->listen_socket = nullptr;
    });

    if (init_future.get()) {
      std::cout << "[INFO] Telemetry server running on port " << std::to_string(port)
                << std::endl;
    } else {
      worker.join();
      throw std::runtime_error("[ERROR] Telemetry server failed to bind to port " +
                               std::to_string(port));
    }
  }

  ~TelemetryServer() {
    if (loop) {
      loop->defer([this]() {
        // Stop listening for new connections
        if (listen_socket) {
          us_listen_socket_close(0, listen_socket);
          listen_socket = nullptr;
        }

        // Force-close all existing connections
        auto clients_copy = clients;
        for (auto* ws : clients_copy) {
          ws->close();
        }
      });
    }

    if (worker.joinable()) {
      worker.join();
    }
  }

  void publish(std::string payload) {
    if (loop == nullptr)
      return;
    loop->defer([this, payload]() {
      if (app != nullptr) {
        app->publish("telemetry_stream", payload, uWS::OpCode::TEXT, false);
      }
    });
  }
};

#endif
