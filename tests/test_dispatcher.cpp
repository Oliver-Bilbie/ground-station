#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <utility>
#include "dispatcher.h"
#include "server.h"

class DummyServer {
 public:
  DummyServer(int port) {}
  const sockaddr_in address() { return sockaddr_in(); }
  template <typename T>
  void send(T packet, sockaddr_in target_address) {}
  template <typename T>
  std::optional<ListenResponse<T>> listen(int timeout_ms) {
    return std::nullopt;
  }
};

class DispatcherTest : public testing::Test {
 protected:
  std::shared_ptr<DummyServer> server = std::make_shared<DummyServer>(3000);
  sockaddr_in address;
  Dispatcher<DummyServer> dispatcher{server, nullptr};

  bool is_retrying(uint64_t satellite_id, uint64_t packet_num) {
    std::lock_guard<std::mutex> lock(dispatcher.mtx);

    auto sat_it = dispatcher.satellite_states.find(satellite_id);
    if (sat_it == dispatcher.satellite_states.end()) {
      return false;
    }

    auto pack_it = sat_it->second.retry_tasks.find(packet_num);
    return pack_it != sat_it->second.retry_tasks.end();
  }

  size_t retry_task_count() {
    std::lock_guard<std::mutex> lock(dispatcher.mtx);
    size_t total = 0;

    for (const auto& [satellite_id, state] : dispatcher.satellite_states) {
      total += state.retry_tasks.size();
    }

    return total;
  }
};

TEST_F(DispatcherTest, ReceiveInOrderDoesNotTriggerRetry) {
  dispatcher.receive(1, 1, address);
  EXPECT_EQ(retry_task_count(), 0);

  dispatcher.receive(1, 2, address);
  EXPECT_EQ(retry_task_count(), 0);
}

TEST_F(DispatcherTest, DetectsSingleGap) {
  dispatcher.receive(1, 1, address);
  dispatcher.receive(1, 3, address);

  ASSERT_EQ(retry_task_count(), 1);
  EXPECT_FALSE(is_retrying(1, 1));
  EXPECT_TRUE(is_retrying(1, 2));
  EXPECT_FALSE(is_retrying(1, 3));
}

TEST_F(DispatcherTest, DetectsMultipleGaps) {
  dispatcher.receive(1, 1, address);
  dispatcher.receive(1, 5, address);

  ASSERT_EQ(retry_task_count(), 3);
  EXPECT_FALSE(is_retrying(1, 1));
  EXPECT_TRUE(is_retrying(1, 2));
  EXPECT_TRUE(is_retrying(1, 3));
  EXPECT_TRUE(is_retrying(1, 4));
  EXPECT_FALSE(is_retrying(1, 5));
}

TEST_F(DispatcherTest, FillsGapSuccessfully) {
  dispatcher.receive(1, 1, address);
  dispatcher.receive(1, 3, address);

  ASSERT_TRUE(is_retrying(1, 2));

  dispatcher.receive(1, 2, address);

  ASSERT_FALSE(is_retrying(1, 2));
}

TEST_F(DispatcherTest, IgnoresDuplicates) {
  dispatcher.receive(1, 1, address);
  dispatcher.receive(1, 1, address);

  EXPECT_EQ(retry_task_count(), 0);
}

TEST_F(DispatcherTest, HandlesMultipleSatellites) {
  dispatcher.receive(1, 1, address);
  ASSERT_EQ(retry_task_count(), 0);
  dispatcher.receive(1, 2, address);
  ASSERT_EQ(retry_task_count(), 0);
  dispatcher.receive(2, 1, address);
  ASSERT_EQ(retry_task_count(), 0);
  dispatcher.receive(1, 3, address);
  ASSERT_EQ(retry_task_count(), 0);
  dispatcher.receive(2, 2, address);
  ASSERT_EQ(retry_task_count(), 0);
  dispatcher.receive(2, 3, address);
  ASSERT_EQ(retry_task_count(), 0);
}
