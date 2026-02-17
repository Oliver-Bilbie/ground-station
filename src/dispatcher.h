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
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "globals.h"
#include "packets.h"

struct RetryState {
  uint16_t retry_count;
  sockaddr_in address;
};

template <typename T>
class Dispatcher {
 public:
  Dispatcher(std::shared_ptr<T> server_ptr) : server(server_ptr), is_running(true) {
    worker = std::thread([this]() {
      while (is_running) {
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [this]() { return (!retry_tasks.empty()) || !is_running; });

          std::vector<uint64_t> timed_out;

          for (const auto& [packet_num, retry_state] : retry_tasks) {
            std::cout << "[INFO] Requesting missing packet: " << packet_num
                      << std::endl;
            server->send(NackPacketData(packet_num).serialize(), retry_state.address);

            if (retry_state.retry_count >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
              timed_out.push_back(packet_num);
            } else {
              retry_tasks.at(packet_num).retry_count++;
            }
          }

          for (const uint64_t& packet_num : timed_out) {
            failed.insert(packet_num);
            retry_tasks.erase(packet_num);
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

  void request_packet(uint64_t packet_number, sockaddr_in address) {
    std::lock_guard<std::mutex> lock(mtx);
    retry_tasks.insert({packet_number, RetryState{0, address}});
    cv.notify_all();
  }

  /**
   * Mark the packet_number as received and request any previous packets that
   * have not yet arrived.
   */
  void receive(uint64_t packet_number, sockaddr_in address) {
    std::lock_guard<std::mutex> lock(mtx);
    retry_tasks.erase(packet_number);
    for (uint64_t n = latest_packet_num + 1; n < packet_number; n++) {
      retry_tasks.insert({n, RetryState{0, address}});
    }
    latest_packet_num = std::max(latest_packet_num, packet_number);
    cv.notify_all();
  }

  std::unordered_set<uint64_t> get_failed() {
    std::lock_guard<std::mutex> lock(mtx);
    return failed;
  };

 private:
  std::unordered_map<uint64_t, RetryState> retry_tasks;
  std::unordered_set<uint64_t> failed;
  uint64_t latest_packet_num = 0;

  std::atomic<bool> is_running;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread worker;

  std::shared_ptr<T> server;

  friend class DispatcherTest;
};

#endif
