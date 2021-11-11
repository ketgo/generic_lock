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

#ifndef GENERIC_LOCK__GENERIC_LOCK_HPP
#define GENERIC_LOCK__GENERIC_LOCK_HPP

#include <thread>
#include <mutex>
#include <unordered_map>

#include <generic_lock/details/dependency_graph.hpp>
#include <generic_lock/details/condition_variable.hpp>
#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/lock_request_queue.hpp>

namespace gl {

/**
 * @brief Contention matrix is a 2-D matrix containing either `true` or `false`
 * as values. Each value represents contention between two lock modes. For
 * example, consider lock modes READ and WRITE for a read-write lock. The
 * contention matrix for the lock will be:
 *
 * @example
 * enum class LockModeType { READ, WRITE=1 };
 * const ContentionMatrix<2> contention_matrix = {{
 *  // LockModeType::READ, LockModeType::WRITE
 *  {{false, true}}, // LockModeType::READ
 *  {{true, true}}  // LockModeType::WRITE
 * }};
 *
 * @tparam modes_count The number of lock modes.
 */
template <size_t modes_count>
using ContentionMatrix = details::ContentionMatrix<modes_count>;

// TODO: Find better way of passing the lock modes and contention matrix to the
// generic lock.

/**
 * @brief The generic mutex class is a synchronization primitive that can be
 * used to protect multiple shared data records from simultaneous access by
 * concurrent theads. The lock expects each data record to be uniquely
 * identified by an identifier. This can be as simple as the key of a value in a
 * hash map, or the position of a value inside a vector or array. It is left to
 * the user to define the identifier. The type of the record identifier can be
 * specified through the template parameter `RecordIdType`. Similarly, each
 * thread is also uniquely identifier using its identifier. The type of thread
 * identifier can be specified using the `ThreadIdType` template parameter. This
 * template parameter is provided to support for transactional access to the
 * shared data records, in which case it would be the transaction identifier
 * type.
 *
 * Unlike the standard mutex, the generic mutex can detect and recover
 * from deadlocks between threads. The recovery policy dictates which thread
 * should be unblocked to recover. The type of policy can be specified through
 * the `Policy` template parameter. By default the thread with the maximum
 * identifier is unblocked.
 *
 * @note The record and thread identifiers, along with the lock mode should be
 * hashable.
 *
 * @tparam RecordIdType The record identifier type.
 * @tparam LockModeType The lock mode type.
 * @tparam modes_count The number of lock modes.
 * @tparam ThreadIdType The thread identifier type. Default set to
 * `std::thread::id`.
 * @tparam timeout The time in milliseconds to wait before checking for
 * deadlock. Default set to `300`.
 */
template <class RecordIdType, class LockModeType, size_t modes_count,
          class ThreadIdType = std::thread::id, size_t timeout = 300>
class GenericLock {
  // Lock request queue type
  typedef details::LockRequestQueue<LockModeType, modes_count, ThreadIdType>
      LockRequestQueue;
  // Lock request group identifier type
  typedef typename LockRequestQueue::LockRequestGroupId LockRequestGroupId;

  // Lock table entry containing queue of lock requests, a condition variable
  // for synchronizing concurrent access, and the currently granted
  // request group identifier.
  struct LockTableEntry {
    // Granted group identifier starts with value of `1` since the first
    // group in the request queue has an identifier of `1`.
    LockTableEntry() : queue(), cv(), granted_group_id(1) {}

    LockRequestQueue queue;
    details::ConditionVariable cv;
    LockRequestGroupId granted_group_id;
  };

  // Table containing lock requests for different records. Each record is
  // associated with its own request queue via its unique key.
  typedef std::unordered_map<RecordIdType, LockTableEntry> LockTable;

  // Lock type.
  typedef std::unique_lock<std::mutex> UniqueLock;

 public:
  /**
   * @brief Construct a new Generic Lock object.
   *
   */
  GenericLock(const ContentionMatrix<modes_count>& contention_matrix)
      : _contention_matrix(contention_matrix) {}

  // Lock not copyable
  GenericLock(const GenericLock& other) = delete;
  // Lock not copy assignable
  GenericLock& operator==(const GenericLock& other) = delete;

  /**
   * @brief Acquire a lock on a record with the given identifier. The calling
   * thread is blocked till the lock is successfully acquired or till the
   * request is denied due to deadlock discovery.
   *
   * @param record_id Constant reference to the record identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier. Default set
   * to `std::this_thread::get_id()`.
   * @returns `true` if the lock is successfully acquired, otherwise `false`.
   */
  bool Lock(const RecordIdType& record_id, const LockModeType& mode,
            const ThreadIdType& thread_id = std::this_thread::get_id()) {
    UniqueLock lock(_latch);

    // Creates a lock table entry if it does not exist already
    auto& entry = _table[record_id];

    // Emplace request in the queue of the record identifier
    auto& group_id = entry.queue.EmplaceLockRequest(mode, thread_id);
    // If the request could not be emplaced then return
    if (group_id == LockRequestQueue::null_group_id) {
      return false;
    }
    // If the emplaced request belong to the granted group then return
    if (group_id == entry.granted_group_id) {
      return true;
    }
    // Either a new group is created or the request is emplaced in the last
    // group. This implies that the thread should wait till the request can be
    // granted.
    entry.cv.Wait(
        lock, _timeout,
        std::bind(&GenericLock::DeadlockCheck, this, record_id, thread_id),
        std::bind(&GenericLock::StopWaiting, this, record_id, thread_id));
  }

  /**
   * @brief
   *
   * @param record_id
   * @param mode
   * @param thread_id
   * @return true
   * @return false
   */
  bool TryLock(const RecordIdType& record_id, const LockModeType& mode,
               const ThreadIdType& thread_id = std::this_thread::get_id());

  /**
   * @brief Unlock an already acquired lock on a record with the given
   * identifier.
   *
   * @param record_id Constant reference to the record identifier.
   * @param thread_id Constant reference to the thread identifier. Default set
   * to `std::this_thread::get_id()`.
   */
  void Unlock(const RecordIdType& record_id,
              const ThreadIdType& thread_id = std::this_thread::get_id());

 private:
  /**
   * @brief Method to check if a thread should stop waiting. A thread can stop
   * waiting if its lock request is granted or if the request is denied due to a
   * deadlock discovery.
   *
   * @param record_id Constant reference to the record identifier.
   * @param thread_id Constant reference to the thread identifier.
   * @returns `true` if thread can stop wating else `false`.
   */
  bool StopWaiting(const RecordIdType& record_id,
                   const ThreadIdType& thread_id) const {
    auto& entry = _table.at(record_id);
    // Stop waiting if the request is granted or it is denied due to discovery
    // of a deadlock.
    return (entry.queue.GetGroupId(thread_id) == entry.granted_group_id) ||
           entry.queue.GetLockRequest(thread_id).IsDenied();
  }

  /**
   * @brief Check for presence of a deadlock and perform recovery actions if it
   * exists.
   *
   * @param record_id Constant reference to the record identifier.
   * @param thread_id Constant reference to the thread identifier.
   */
  void DeadlockCheck(const RecordIdType& record_id,
                     const ThreadIdType& thread_id) {}

  // Lock mode contention matrix
  const ContentionMatrix<modes_count> _contention_matrix;
  // Thread wait timeout to check for deadlocks.
  static constexpr std::chrono::milliseconds _timeout{timeout};
  // Latch for atomic modification of the lock.
  std::mutex _latch;
  // Lock table recording state of the lock.
  LockTable _table;
  // Dependency graph between different lock requests.
  details::DependencyGraph<ThreadIdType> _dependency_graph;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
