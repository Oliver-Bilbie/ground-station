#ifndef LATENCY_TRACKER_H
#define LATENCY_TRACKER_H

#include <netinet/in.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "globals.h"
#include "rolling_avg.h"
#include "telemetry_server.h"
#include "timer.h"

struct LatencyState {
  uint64_t latest_timestamp;
  uint64_t telemetry_timestamp;
  RollingAverage<uint64_t> latency;
};

class LatencyTracker {
 public:
  LatencyTracker(std::shared_ptr<TelemetryServer> telemetry_server)
      : telemetry(telemetry_server), is_running(true) {
    if (telemetry_server == nullptr) {
      std::cout << "[WARN] Latency Tracker was initialized without a telemetry server"
                << std::endl;
    }

    // Spawn a thread to periodically check for disconnections
    worker = std::thread([this]() {
      while (is_running) {
        std::vector<uint64_t> timed_out;
        std::vector<std::function<void(uint64_t)>> disconnect_handlers_copy;

        {
          std::unique_lock<std::mutex> lock(mtx);

          for (const auto& [satellite_id, state] : satellites) {
            timer.set(state.latest_timestamp);
            if (timer.elapsed() > MAX_LATENCY_MS) {
              timed_out.push_back(satellite_id);
            }
          }

          if (!timed_out.empty()) {
            disconnect_handlers_copy = disconnect_handlers;
            for (const auto& satellite_id : timed_out) {
              satellites.erase(satellite_id);
            }
          }
        }

        for (const auto& satellite_id : timed_out) {
          std::cout << "[INFO] Connection to satellite " << satellite_id
                    << " has timed out" << std::endl;

          if (telemetry != nullptr) {
            std::ostringstream json_oss;
            json_oss << "{\"event\": \"disconnect\", \"satellite_id\": \""
                     << satellite_id << "\"}" << std::endl;
            telemetry->publish(json_oss.str());
          }

          for (const auto& handler : disconnect_handlers_copy) {
            handler(satellite_id);
          }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MAX_LATENCY_MS / 10));
      }
    });
  }

  ~LatencyTracker() {
    is_running = false;
    if (worker.joinable()) {
      worker.join();
    }
  }

  void add_contribution(uint64_t satellite_id, uint64_t timestamp) {
    std::ostringstream telemetry_json;

    {
      std::lock_guard<std::mutex> lock(mtx);

      timer.set(timestamp);
      uint64_t elapsed = timer.elapsed();

      auto [it, inserted] = satellites.try_emplace(
          satellite_id, LatencyState{0, 0, RollingAverage<uint64_t>(50)});
      it->second.latency.add_contribution(elapsed);
      it->second.latest_timestamp = timestamp;

      if (telemetry != nullptr) {
        uint64_t last_broadcast = it->second.telemetry_timestamp;
        timer.set(last_broadcast);
        if (timer.elapsed() > LATENCY_BROADCAST_PERIOD_MS) {
          it->second.telemetry_timestamp = timestamp;
          telemetry_json << "{\"event\": \"latency\", \"satellite_id\": \""
                         << satellite_id
                         << "\", \"latency\": " << it->second.latency.get_value()
                         << "}";
        }
      }
    }

    if (!telemetry_json.str().empty()) {
      telemetry->publish(telemetry_json.str());
    }
  }

  std::optional<uint64_t> get(uint64_t satellite_id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = satellites.find(satellite_id);
    if (it == satellites.end()) {
      std::cout << "[WARN] Latency data missing for satellite " << satellite_id
                << std::endl;
      return std::nullopt;
    }

    return it->second.latency.get_value();
  }

  void on_disconnect(std::function<void(uint64_t)> disconnect_handler) {
    std::lock_guard<std::mutex> lock(mtx);
    disconnect_handlers.push_back(std::move(disconnect_handler));
  };

 private:
  std::unordered_map<uint64_t, LatencyState> satellites;
  std::vector<std::function<void(uint64_t)>> disconnect_handlers;
  Timer timer;

  std::shared_ptr<TelemetryServer> telemetry;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  friend class LatencyTrackerTest;
};

#endif
