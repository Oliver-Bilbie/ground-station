#include "dispatcher.h"
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include "packets.h"
#include "simulate_noise.h"

Dispatcher::Dispatcher(uint16_t mr, int fd)
    : max_retries(mr), file_descriptor(fd), is_running(true), is_ready(false) {
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

          if (retry_count >= max_retries) {
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
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

void Dispatcher::mark_received(uint64_t packet_number) {
  std::lock_guard<std::mutex> lock(mtx);
  retry_counts.erase(packet_number);
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
