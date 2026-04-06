#ifndef LATENCY_TRACKER_H
#define LATENCY_TRACKER_H

#include <netinet/in.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>
#include "globals.h"
#include "rolling_avg.h"
#include "telemetry_server.h"
#include "timer.h"

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
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait_for(lock, std::chrono::milliseconds(MAX_LATENCY_MS / 100), [this]() {
            return !is_running;
          });

          std::vector<uint64_t> timed_out;

          for (const auto& [satellite_id, timestamp] : latest_timestamps) {
            timer.set(timestamp);
            if (timer.elapsed() > MAX_LATENCY_MS) {
              timed_out.push_back(satellite_id);

              std::cout << "[INFO] Connection to satellite " << satellite_id
                        << " has timed out" << std::endl;

              if (telemetry != nullptr) {
                std::ostringstream json_oss;
                json_oss << "{\"event\": \"disconnect\", \"satellite_id\": "
                         << satellite_id << "}" << std::endl;
                telemetry->publish(json_oss.str());
              }
            }
          }

          for (const auto& satellite_id : timed_out) {
            latest_timestamps.erase(satellite_id);
            telemetry_timestamps.erase(satellite_id);
            latencies.erase(satellite_id);
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(MAX_LATENCY_MS / 10));
      }
    });
  }

  ~LatencyTracker() {
    is_running = false;
    cv.notify_all();
    if (worker.joinable()) {
      worker.join();
    }
  }

  void add_contribution(uint64_t satellite_id, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(mtx);

    timer.set(timestamp);
    uint64_t elapsed = timer.elapsed();

    latest_timestamps[satellite_id] = timestamp;

    auto [it, inserted] = latencies.try_emplace(satellite_id, 50);
    it->second.add_contribution(elapsed);

    if (telemetry != nullptr) {
      uint64_t last_broadcast = telemetry_timestamps[satellite_id];
      timer.set(last_broadcast);
      if (timer.elapsed() > LATENCY_BROADCAST_PERIOD_MS) {
        telemetry_timestamps[satellite_id] = timestamp;
        std::ostringstream json_oss;
        json_oss << "{\"event\": \"latency\", \"satellite_id\": " << satellite_id
                 << ", \"latency\": " << it->second.get_value() << "}";
        telemetry->publish(json_oss.str());
      }
    }

    cv.notify_all();
  }

  std::optional<uint64_t> get(uint64_t satellite_id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = latencies.find(satellite_id);

    cv.notify_all();

    if (it != latencies.end()) {
      return it->second.get_value();
    }

    std::cout << "[WARN] Latency data missing for satellite " << satellite_id
              << std::endl;
    return std::nullopt;
  }

 private:
  std::unordered_map<uint64_t, uint64_t> latest_timestamps;
  std::unordered_map<uint64_t, uint64_t> telemetry_timestamps;
  std::unordered_map<uint64_t, RollingAverage<uint64_t>> latencies;
  Timer timer;

  std::shared_ptr<TelemetryServer> telemetry;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  friend class LatencyTrackerTest;
};

#endif
