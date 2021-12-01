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
#include <unordered_map>

#include <generic_lock/generic_lock.hpp>
#include <generic_lock/generic_mutex.hpp>

#include "event_log.hpp"

using namespace gl;
using namespace std::chrono_literals;

class GenericMutexTestFixture : public ::testing::Test {
 protected:
  typedef size_t RecordId;
  typedef size_t ThreadId;
  static constexpr auto wait_between_operations = 5ms;
  static constexpr size_t timeout_ms = 1;

  // ----------------------------
  enum class LockMode { READ, WRITE };
  const ContentionMatrix<2> contention_matrix = {
      {{{false, true}}, {{true, true}}}};

  typedef GenericMutex<RecordId, LockMode, 2, ThreadId, timeout_ms>
      GenericMutexType;
  GenericMutexType mutex = {contention_matrix};
  // ----------------------------

  // ----------------------------
  // TODO: Use a mock instead of GenericLock.
  typedef GenericLock<GenericMutexType> LockGuard;
  // ----------------------------

  // ---------------------------
  // Operation record
  struct OpRecord {
    ThreadId thread_id;
    enum class Type { READ, WRITE } type;
    RecordId record_id;
    char value;

    OpRecord() = default;
    OpRecord(const ThreadId& thread_id, const Type& type,
             const RecordId& record_id, const char& value)
        : thread_id(thread_id),
          type(type),
          record_id(record_id),
          value(value) {}
  };
  // Group of operation records
  typedef std::vector<OpRecord> OpRecordGroup;
  // Stores operations performed by multiple threads in chronological order
  typedef EventLog<OpRecord> OpLog;
  OpLog op_log;
  // Records shared by multiple threads
  typedef std::unordered_map<RecordId, char> Records;
  Records records = {{0, '0'}, {1, '1'}, {2, '2'}, {3, '3'}, {4, '4'}};
  // ---------------------------

  // ---------------------------
  // Lock request data
  struct LockRequest {
    RecordId record_id;
    LockMode mode;
    ThreadId thread_id;
    size_t seq;
    bool granted;

    LockRequest(const RecordId& record_id, const LockMode& mode,
                const ThreadId& thread_id, const size_t& seq,
                const bool& granted)
        : record_id(record_id),
          mode(mode),
          thread_id(thread_id),
          seq(seq),
          granted(granted) {}
  };
  // Stores result for lock request by multiple thread in chronological order
  typedef EventLog<LockRequest> LockResultsLog;
  LockResultsLog lock_results_log;
  // ---------------------------

  void SetUp() override {}
  void TearDown() override {}

 public:
  /**
   * @brief Run test in a thread by performing the group og operations specified
   * in the given operation record group.
   *
   * @note The method implements 2PL.
   *
   * @param op_record_group Constant reference to the operation record group.
   */
  void TestThread(const OpRecordGroup& op_record_group) {
    std::vector<LockGuard> guards;

    for (size_t i = 0; i < op_record_group.size(); ++i) {
      auto& op_record = op_record_group[i];
      if (op_record.type == OpRecord::Type::READ) {
        guards.emplace_back(mutex, op_record.record_id, LockMode::READ,
                            op_record.thread_id);
        // Log lock request
        lock_results_log.emplace(op_record.record_id, LockMode::READ,
                                 op_record.thread_id, i, bool(guards.back()));
        // Perform operation
        auto value = records.at(op_record.record_id);
        // Log operation
        op_log.emplace(op_record.thread_id, OpRecord::Type::READ,
                       op_record.record_id, value);
      } else {
        guards.emplace_back(mutex, op_record.record_id, LockMode::WRITE,
                            op_record.thread_id);
        // Store lock result
        lock_results_log.emplace(op_record.record_id, LockMode::WRITE,
                                 op_record.thread_id, i, bool(guards.back()));
        // Perform operation
        records[op_record.record_id] = op_record.value;
        // Log operation
        op_log.emplace(op_record.thread_id, OpRecord::Type::WRITE,
                       op_record.record_id, op_record.value);
      }
      // Wait between operations
      std::this_thread::sleep_for(wait_between_operations);
    }
  }
};

TEST_F(GenericMutexTestFixture, TestLockUnlock) {
  const std::vector<OpRecordGroup> op_groups = {
      {{1, OpRecord::Type::READ, 0, ' '}},
      {{2, OpRecord::Type::READ, 0, ' '}},
      {{3, OpRecord::Type::READ, 1, ' '}},
      {{4, OpRecord::Type::WRITE, 0, 'd'}},
      {{5, OpRecord::Type::READ, 0, ' '}},
      {{6, OpRecord::Type::READ, 1, ' '}},
      {{7, OpRecord::Type::WRITE, 0, 'a'}},
      {{8, OpRecord::Type::WRITE, 1, 'e'}},
      {{9, OpRecord::Type::WRITE, 0, 'f'}},
      {{10, OpRecord::Type::READ, 0, ' '}},
      {{11, OpRecord::Type::READ, 0, ' '}},
      {{12, OpRecord::Type::READ, 1, ' '}}};
  std::vector<std::thread> threads(op_groups.size());

  // Start threads
  for (size_t idx = 0; idx < op_groups.size(); ++idx) {
    threads[idx] = std::thread(&GenericMutexTestFixture::TestThread, this,
                               std::ref(op_groups[idx]));
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }

  // Assert all lock request results
  for (auto& lock_results : lock_results_log) {
    ASSERT_TRUE(lock_results.granted);
  }

  // Assert outcome based on operation log
  Records _records = {{0, '0'}, {1, '1'}};
  for (auto& op_record : op_log) {
    if (op_record.type == OpRecord::Type::READ) {
      ASSERT_EQ(_records[op_record.record_id], op_record.value);
    } else {
      _records[op_record.record_id] = op_record.value;
    }
  }
}

TEST_F(GenericMutexTestFixture, TestDedlockRecovery) {
  const std::vector<OpRecordGroup> op_groups = {
      {{1, OpRecord::Type::WRITE, 0, 'a'},
       {1, OpRecord::Type::WRITE, 1, 'a'},
       {1, OpRecord::Type::WRITE, 2, 'a'},
       {1, OpRecord::Type::WRITE, 3, 'a'},
       {1, OpRecord::Type::WRITE, 4, 'a'}},
      {{2, OpRecord::Type::WRITE, 4, 'b'},
       {2, OpRecord::Type::WRITE, 3, 'b'},
       {2, OpRecord::Type::WRITE, 2, 'b'},
       {2, OpRecord::Type::WRITE, 1, 'b'},
       {2, OpRecord::Type::WRITE, 0, 'b'}}};

  std::vector<std::thread> threads(op_groups.size());

  // Start threads
  for (size_t idx = 0; idx < op_groups.size(); ++idx) {
    threads[idx] = std::thread(&GenericMutexTestFixture::TestThread, this,
                               std::ref(op_groups[idx]));
  }

  // Wait till all threads finish
  for (auto& thread : threads) {
    thread.join();
  }

  // Assert that one of the lock request by thread 2 is denied due to deadlock
  // with thread 1. Thread 2 is denied because of the SelectMaxPolicy.
  bool thread_1_all_granted = true;
  bool thread_2_all_granted = true;
  for (auto& lock_results : lock_results_log) {
    if (lock_results.thread_id == 1) {
      thread_1_all_granted = thread_1_all_granted && lock_results.granted;
    }
    if (lock_results.thread_id == 2) {
      thread_2_all_granted = thread_2_all_granted && lock_results.granted;
    }
  }
  ASSERT_TRUE(thread_1_all_granted);
  ASSERT_FALSE(thread_2_all_granted);

  // Assert outcome based on operation log
  Records _records = {{0, '0'}, {1, '1'}};
  for (auto& op_record : op_log) {
    if (op_record.type == OpRecord::Type::READ) {
      ASSERT_EQ(_records[op_record.record_id], op_record.value);
    } else {
      _records[op_record.record_id] = op_record.value;
    }
  }
}