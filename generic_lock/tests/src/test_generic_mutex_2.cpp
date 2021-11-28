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
  typedef LoggedMap<ThreadId, int, char> Records;
  Records records = {{0, '0'}, {1, '1'}};
  // ---------------------------

  void SetUp() override {}
  void TearDown() override {}

 public:
  /**
   * @brief Run test in a thread by performing the operation specified in the
   * given operation record.
   *
   * @param op_record Constant reference to the operation record.
   */
  void TestThreadLockUnlock(Records::OpRecord& op_record) {
    if (op_record.type == Records::OpRecord::Type::READ) {
      GenericLockType guard(mutex, op_record.key, LockMode::READ,
                            op_record.thread_id);
      // Asserts that the lock request is successful.
      ASSERT_TRUE(guard);
      op_record.value = records.Get(op_record.thread_id, op_record.key);
    } else {
      GenericLockType guard(mutex, op_record.key, LockMode::WRITE,
                            op_record.thread_id);
      // Asserts that the lock request is successful.
      ASSERT_TRUE(guard);
      records.Set(op_record.thread_id, op_record.key, op_record.value);
    }
  }
};

TEST_F(GenericMutex2TestFixture, TestLockUnlock) {
  std::vector<Records::OpRecord> ops = {
      {1, Records::OpRecord::Type::READ, 0, ' '},
      {2, Records::OpRecord::Type::READ, 0, ' '},
      {3, Records::OpRecord::Type::READ, 1, ' '},
      {4, Records::OpRecord::Type::WRITE, 0, 'd'},
      {5, Records::OpRecord::Type::READ, 0, ' '},
      {6, Records::OpRecord::Type::READ, 1, ' '},
      {7, Records::OpRecord::Type::WRITE, 0, 'a'},
      {8, Records::OpRecord::Type::WRITE, 1, 'e'},
      {9, Records::OpRecord::Type::WRITE, 0, 'f'},
      {10, Records::OpRecord::Type::READ, 0, ' '},
      {11, Records::OpRecord::Type::READ, 0, ' '},
      {12, Records::OpRecord::Type::READ, 1, ' '}};
  std::vector<std::thread> threads(ops.size());

  // Start threads
  for (size_t idx = 0; idx < ops.size(); ++idx) {
    threads[idx] = std::thread(&GenericMutex2TestFixture::TestThreadLockUnlock,
                               this, std::ref(ops[idx]));
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }

  // Assert outcome based on operation log
  std::unordered_map<int, char> _records = {{0, '0'}, {1, '1'}};
  for (auto& op_record : records.OperationLog()) {
    if (op_record.type == Records::OpRecord::Type::READ) {
      ASSERT_EQ(_records[op_record.key], op_record.value);
    } else {
      _records[op_record.key] = op_record.value;
    }
  }
}