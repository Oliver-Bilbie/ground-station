#include "dispatcher.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include "globals.h"
#include "packets.h"
#include "simulate_noise.h"

Dispatcher::Dispatcher(int fd)
    : file_descriptor(fd), is_running(true), is_ready(false) {
  worker = std::thread([this]() {
    while (is_running) {
      {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
          return (is_ready && !retry_counts.empty()) || !is_running;
        });

        std::vector<uint64_t> timed_out;

        for (const auto& [packet_num, retry_count] : retry_counts) {
          std::cout << "[INFO] Requesting missing packet: " << packet_num << std::endl;
          send_through_space(
              NackPacketData(packet_num).serialize(), file_descriptor, target_address);

          if (retry_count >= TIMEOUT_MS / CYCLE_LENGTH_MS) {
            timed_out.push_back(packet_num);
          } else {
            retry_counts.at(packet_num)++;
          }
        }

        for (const uint64_t& packet_num : timed_out) {
          failed.insert(packet_num);
          retry_counts.erase(packet_num);
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_LENGTH_MS));
    }
  });
}

Dispatcher::~Dispatcher() {
  is_running = false;
  cv.notify_all();
  if (worker.joinable()) {
    worker.join();
  }
}

void Dispatcher::request_packet(uint64_t packet_number) {
  std::lock_guard<std::mutex> lock(mtx);
  retry_counts.insert({packet_number, 0});
  cv.notify_all();
}

void Dispatcher::receive(uint64_t packet_number) {
  std::lock_guard<std::mutex> lock(mtx);
  retry_counts.erase(packet_number);
  for (int n = latest_packet_num + 1; n < packet_number; n++) {
    retry_counts.insert({packet_number, 0});
  }
  latest_packet_num = std::max(latest_packet_num, packet_number);
  cv.notify_all();
}

std::unordered_set<uint64_t> Dispatcher::get_failed() {
  std::lock_guard<std::mutex> lock(mtx);
  return failed;
};

void Dispatcher::set_target_address(sockaddr_in addr) {
  std::lock_guard<std::mutex> lock(mtx);
  target_address = addr;
  is_ready = true;
}
