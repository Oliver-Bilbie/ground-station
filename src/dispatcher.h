#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <netinet/in.h>
#include <algorithm>
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
#include "timer.h"

struct RetryState {
  uint16_t retry_count;
  uint64_t retry_after_time;
  uint16_t retry_delay;
  sockaddr_in address;
};

struct DispatcherState {
  uint64_t latest_packet = 0;
  uint16_t failed_count = 0;
  // Maps packet_number -> RetryState
  std::unordered_map<uint64_t, RetryState> retry_tasks;
};

struct DispatcherRetryItem {
  uint64_t satellite_id;
  uint64_t packet_number;
  sockaddr_in address;
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
        std::vector<DispatcherRetryItem> retry_items;

        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait_for(lock, std::chrono::milliseconds(CYCLE_LENGTH_MS), [this]() {
            return !is_running;
          });

          if (!is_running)
            break;

          uint64_t current_time = timer.elapsed();

          for (auto& [satellite_id, state] : satellite_states) {
            std::vector<uint64_t> timed_out_packets;

            for (auto& [packet_number, retry_state] : state.retry_tasks) {
              // Skip if it's not time to retry this packet yet
              if (current_time < retry_state.retry_after_time) {
                continue;
              }

              retry_items.push_back(DispatcherRetryItem{
                  satellite_id, packet_number, retry_state.address});

              retry_state.retry_count++;
              if (retry_state.retry_count >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
                timed_out_packets.push_back(packet_number);
              } else {
                retry_state.retry_delay *= 2;
                retry_state.retry_after_time = current_time + retry_state.retry_delay;
              }
            }

            for (uint64_t pkt_num : timed_out_packets) {
              state.retry_tasks.erase(pkt_num);
              state.failed_count++;
            }
          }
        }

        for (const auto& item : retry_items) {
          std::cout << "[INFO] Requesting missing packet: " << item.packet_number
                    << " from satellite " << item.satellite_id << std::endl;

          if (telemetry != nullptr) {
            std::ostringstream json_oss;
            json_oss << "{\"event\": \"dropped_packet\", \"satellite_id\": \""
                     << item.satellite_id << "\", \"packet_number\": \""
                     << item.packet_number << "\"}";
            telemetry->publish(json_oss.str());
          }

          server->send(NackPacketData(item.packet_number).serialize(), item.address);
        }
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

    auto& state = satellite_states[satellite_id];

    state.retry_tasks.erase(packet_number);

    if (state.latest_packet == 0 && packet_number != 0) {
      state.latest_packet = packet_number;
      return;
    }

    uint64_t initial_retry_time = timer.elapsed() + CYCLE_LENGTH_MS;
    for (uint64_t n = state.latest_packet + 1; n < packet_number; n++) {
      state.retry_tasks[n] = {0, initial_retry_time, CYCLE_LENGTH_MS, address};
    }

    state.latest_packet = std::max(state.latest_packet, packet_number);
    cv.notify_all();
  }

  void on_disconnect(uint64_t satellite_id) {
    uint64_t failed_count;
    {
      std::lock_guard<std::mutex> lock(mtx);
      auto it = satellite_states.find(satellite_id);
      if (it == satellite_states.end()) {
        return;
      }

      failed_count = it->second.failed_count;
      satellite_states.erase(satellite_id);
    }

    if (telemetry != nullptr) {
      std::ostringstream json_oss;
      json_oss << "{\"event\": \"lost_packet_report\", \"satellite_id\": \""
               << satellite_id << "\", \"lost_packet_total\": " << failed_count << "}";
      telemetry->publish(json_oss.str());
    }
  }

 private:
  std::unordered_map<uint64_t, DispatcherState> satellite_states;
  Timer timer;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  std::shared_ptr<T> server;
  std::shared_ptr<TelemetryServer> telemetry;

  friend class DispatcherTest;
};

#endif
