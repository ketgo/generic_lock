// Copyright 2021 Ketan Goyal
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief Uint Test Condition Variable
 *
 */

#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include <generic_lock/details/condition_variable.hpp>

using namespace gl::details;
using namespace std::chrono_literals;

class ConditionVariableTestFixture : public ::testing::Test {
 protected:
  template <class T>
  class TestQueue {
   public:
    TestQueue() : _mutex(), _cv(), callback_called(false), queue() {}

    T Get() const {
      std::unique_lock<std::mutex> lock(_mutex);

      _cv.Wait(
          lock, timeout, [&]() { callback_called = true; },
          [&]() { return !queue.empty(); });
      return queue.back();
    }

    void Put(const T& value) {
      std::unique_lock<std::mutex> lock(_mutex);

      queue.push_back(value);
      _cv.NotifyAll();
    }

    std::vector<T> queue;
    mutable bool callback_called;
    static constexpr auto timeout = 5ms;

   private:
    mutable std::mutex _mutex;
    mutable ConditionVariable _cv;
  };
  TestQueue<int> queue;

  void SetUp() override {}
  void TearDown() override {}
};

/**
 * @note The following case only tests for the new funcionality added to
 * std::condition_variable.
 *
 */
TEST_F(ConditionVariableTestFixture, TestWait) {
  std::thread thread_a, thread_b;
  int value;

  ASSERT_FALSE(queue.callback_called);

  // Start threads
  thread_a = std::thread([&]() {
    std::this_thread::sleep_for(2 * queue.timeout);
    queue.Put(10);
  });
  thread_b = std::thread([&]() { value = queue.Get(); });

  // Wait till all threads to finish
  thread_a.join();
  thread_b.join();

  ASSERT_EQ(value, 10);
  ASSERT_TRUE(queue.callback_called);
}
