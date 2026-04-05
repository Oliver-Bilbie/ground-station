#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <netinet/in.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "globals.h"
#include "packets.h"
#include "telemetry_server.h"

struct RetryState {
  uint16_t retry_count;
  sockaddr_in address;
};

struct PairHash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};

template <typename T>
class Dispatcher {
 public:
  Dispatcher(std::shared_ptr<T> server_ptr,
             std::shared_ptr<TelemetryServer> telemetry_server)
      : server(server_ptr), telemetry(telemetry_server), is_running(true) {
    if (telemetry_server == nullptr) {
      std::cout << "[WARN] Dispatcher was initialized without a telemetry server"
                << std::endl;
    }

    worker = std::thread([this]() {
      while (is_running) {
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [this]() { return (!retry_tasks.empty()) || !is_running; });

          std::vector<std::pair<uint64_t, uint64_t>> timed_out;

          for (const auto& [retry_key, retry_state] : retry_tasks) {
            std::cout << "[INFO] Requesting missing packet: " << retry_key.second
                      << " from satellite " << retry_key.first << std::endl;

            if (telemetry != nullptr) {
              std::ostringstream json_oss;
              json_oss << "{\"event\": \"dropped_packet\", \"satellite_id\": "
                       << retry_key.first << "}" << std::endl;
              telemetry->publish(json_oss.str());
            }

            server->send(NackPacketData(retry_key.second).serialize(),
                         retry_state.address);

            if (retry_state.retry_count >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
              timed_out.push_back(retry_key);
            } else {
              retry_tasks.at(retry_key).retry_count++;
            }
          }

          for (const auto& packet_key : timed_out) {
            failed.insert(packet_key);
            retry_tasks.erase(packet_key);
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_LENGTH_MS));
      }
    });
  }

  ~Dispatcher() {
    is_running = false;
    cv.notify_all();
    if (worker.joinable()) {
      worker.join();
    }
  }

  /**
   * Mark the packet_number as received and request any previous packets that
   * have not yet arrived.
   */
  void receive(uint64_t satellite_id, uint64_t packet_number, sockaddr_in address) {
    std::lock_guard<std::mutex> lock(mtx);

    retry_tasks.erase(std::pair{satellite_id, packet_number});
    auto latest_packet_it = latest_packets.find(satellite_id);
    if (latest_packet_it == latest_packets.end()) {
      latest_packets[satellite_id] = packet_number;
      return;
    }

    for (uint64_t n = latest_packet_it->second + 1; n < packet_number; n++) {
      retry_tasks.insert({std::pair{satellite_id, n}, RetryState{0, address}});
    }
    latest_packet_it->second = std::max(latest_packet_it->second, packet_number);
    cv.notify_all();
  }

  std::unordered_map<uint64_t, uint64_t> get_failed() {
    std::lock_guard<std::mutex> lock(mtx);
    return failed;
  };

 private:
  std::unordered_map<uint64_t, uint64_t> latest_packets;
  std::unordered_map<std::pair<uint64_t, uint64_t>, RetryState, PairHash> retry_tasks;
  std::unordered_map<uint64_t, uint64_t> failed;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  std::shared_ptr<T> server;
  std::shared_ptr<TelemetryServer> telemetry;

  friend class DispatcherTest;
};

#endif
