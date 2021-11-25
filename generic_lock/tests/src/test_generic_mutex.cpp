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
 * @brief Uint Test Generic Mutex
 *
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include <generic_lock/generic_mutex.hpp>
#include <generic_lock/generic_lock.hpp>

using namespace gl;
using namespace std::chrono_literals;

class GenericMutexTestFixture : public ::testing::Test {
 protected:
  typedef size_t RecordId;
  typedef size_t ThreadId;

  static constexpr auto thread_sleep = 5ms;
  static constexpr size_t timeout_ms = 1;

  enum class LockMode { READ, WRITE };
  const ContentionMatrix<2> contention_matrix = {
      {{{false, true}}, {{true, true}}}};

  typedef GenericMutex<RecordId, LockMode, 2, ThreadId, timeout_ms>
      GenericMutexType;
  GenericMutexType mutex = {contention_matrix};
  typedef GenericLock<GenericMutexType> GenericLockType;

  typedef std::unordered_map<int, char> Records;
  struct Op {
    Op(const ThreadId& thread_id = 0, const bool& write = false,
       const int& key = 0, const char& value = 0)
        : thread_id(thread_id), write(write), key(key), value(value) {}

    ThreadId thread_id;
    bool write;
    int key;
    char value;
  };
  typedef std::vector<Op> OpGroup;

  void SetUp() override {}
  void TearDown() override {}

 public:
  /**
   * @brief Run test in a thread by performing the given operation.
   *
   * @param op Constant reference to the operation to perform.
   * @param records Reference to the records to be operated.
   */
  void TestThreadLockUnlock(const Op& op, Records& records) {
    std::vector<GenericLockType> guards;

    if (op.write) {
      guards.emplace_back(mutex, op.key, LockMode::WRITE, op.thread_id);
      // Asserts that the lock request is successful.
      ASSERT_TRUE(guards.back());
      records[op.key] = op.value;
    } else {
      guards.emplace_back(mutex, op.key, LockMode::READ, op.thread_id);
      ASSERT_EQ(records[op.key], op.value);
    }
    std::this_thread::sleep_for(thread_sleep);
  }

  /**
   * @brief Run test in a thread by performing the given operation.
   *
   * @param op_group Constant reference to the group of operations to perform.
   * @param records Reference to the records to be operated.
   */
  void TestThreadDeadlock(const OpGroup& op_group, Records& records) {
    std::vector<GenericLockType> guards;

    for (auto& op : op_group) {
      if (op.write) {
        guards.emplace_back(mutex, op.key, LockMode::WRITE, op.thread_id);
        // Check for the transaction and operation which causes deadlock.
        if (op.thread_id == 5 && op.key == 0) {
          // Asserts that the lock request is denied.
          ASSERT_FALSE(guards.back());
        } else {
          // Asserts that the lock request is successful.
          ASSERT_TRUE(guards.back());
        }
        records[op.key] = op.value;
      } else {
        guards.emplace_back(mutex, op.key, LockMode::READ, op.thread_id);
        ASSERT_EQ(records[op.key], op.value);
      }
      std::this_thread::sleep_for(thread_sleep);
    }
  }
};

TEST_F(GenericMutexTestFixture, TestLockUnlock) {
  Records records = {{0, 'a'}, {1, 'b'}};
  const std::vector<Op> ops = {
      {1, false, 0, 'a'},  {2, false, 0, 'a'},  {3, false, 1, 'b'},
      {4, true, 0, 'd'},   {5, false, 0, 'd'},  {6, false, 1, 'b'},
      {7, true, 0, 'a'},   {8, true, 1, 'e'},   {9, true, 0, 'f'},
      {10, false, 0, 'f'}, {11, false, 0, 'f'}, {12, false, 1, 'e'}};
  std::vector<std::thread> threads(ops.size());

  // Start threads
  for (size_t idx = 0; idx < ops.size(); ++idx) {
    threads[idx] = std::thread(&GenericMutexTestFixture::TestThreadLockUnlock,
                               this, ops[idx], std::ref(records));
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }
}

/*
TEST_F(GenericMutexTestFixture, TestDeadlock) {
  Records records = {{0, '0'}, {1, '1'}, {2, '2'}};
  // Transaction 5 causes deadlock with 2
  const std::vector<OpGroup> op_groups = {
      {{1, true, 0, 'a'}},
      {{2, true, 0, 'b'}, {2, true, 1, 'b'}, {2, true, 2, 'b'}},
      {{3, true, 0, 'd'}},
      {{4, true, 0, 'e'}, {4, true, 1, 'e'}},
      {{5, true, 1, 'f'}, {5, true, 0, 'f'}},
      {{6, true, 1, 'g'}},
      {{7, true, 1, 'h'}, {7, true, 2, 'h'}}};
  std::vector<std::thread> threads(op_groups.size());

  // Start threads
  for (size_t idx = 0; idx < op_groups.size(); ++idx) {
    threads[idx] = std::thread(&GenericMutexTestFixture::TestThreadDeadlock,
                               this, op_groups[idx], std::ref(records));
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }
}*/