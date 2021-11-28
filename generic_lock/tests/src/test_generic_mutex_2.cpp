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

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include <generic_lock/generic_lock.hpp>
#include <generic_lock/generic_mutex.hpp>

#include "logged_map.hpp"

using namespace gl;
using namespace std::chrono_literals;

class GenericMutex2TestFixture : public ::testing::Test {
 protected:
  typedef size_t RecordId;
  typedef size_t ThreadId;
  static constexpr auto thread_sleep = 5ms;
  static constexpr size_t timeout_ms = 1;

  // ----------------------------
  enum class LockMode { READ, WRITE };
  const ContentionMatrix<2> contention_matrix = {
      {{{false, true}}, {{true, true}}}};

  typedef GenericMutex<RecordId, LockMode, 2, ThreadId, timeout_ms>
      GenericMutexType;
  GenericMutexType mutex = {contention_matrix};
  typedef GenericLock<GenericMutexType> GenericLockType;
  // ----------------------------

  // ---------------------------
  LoggedMap<ThreadId, int, char> records = {{1, '1'}, {2, '2'}};
  // ---------------------------

  void SetUp() override {}
  void TearDown() override {}

 public:
  void TestThreadLockUnlock() {}
};

TEST_F(GenericMutex2TestFixture, TestLockUnlock) {
  std::vector<std::thread> threads(2);

  // Start threads
  for (size_t idx = 0; idx < 2; ++idx) {
    threads[idx] =
        std::thread(&GenericMutex2TestFixture::TestThreadLockUnlock, this);
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }
}