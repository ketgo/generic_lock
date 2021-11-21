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

#ifndef GENERIC_LOCK__GENERIC_MUTEX_HPP
#define GENERIC_LOCK__GENERIC_MUTEX_HPP

#include <thread>
#include <mutex>
#include <unordered_map>

#include <generic_lock/details/dependency_graph.hpp>
#include <generic_lock/details/condition_variable.hpp>
#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/lock_request_queue.hpp>

#include <generic_lock/recovery_policy.hpp>

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

// TODO: Use a different name instead of `ThreadIdType` (how about
// `ExecutionIdType`?). That will enable removing the default value
// `std::this_thread::get_id()`. Let the user decide if they which to use the
// thread identifier explicitly.

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
 * @tparam Policy Deadlock recovery policy type. Default set to
 * `SelectMaxPolicy<ThreadIdType>`.
 */
template <class RecordIdType, class LockModeType, size_t modes_count,
          class ThreadIdType = std::thread::id, size_t timeout = 300,
          class Policy = SelectMaxPolicy<ThreadIdType>>
class GenericMutex {
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
  // Mutex traits
  typedef RecordIdType record_id_t;
  typedef LockModeType lok_mode_t;
  typedef ThreadIdType thread_id_t;

  /**
   * @brief Construct a new Generic Lock object.
   *
   */
  GenericMutex(const ContentionMatrix<modes_count>& contention_matrix)
      : _contention_matrix(contention_matrix) {}

  // Mutex not copyable
  GenericMutex(const GenericMutex& other) = delete;
  // Mutex not copy assignable
  GenericMutex& operator=(const GenericMutex& other) = delete;

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
    // If the emplaced request belong to the granted group then return as the
    // lock has been granted successfully.
    if (group_id == entry.granted_group_id) {
      return true;
    }

    // Either a new group is created or the request is emplaced in the last
    // group. This implies that the thread needs to wait till the request can be
    // granted. Furthermore, the thread is dependent on the prior requests to be
    // granted. We thus have to update the dependency graph and put the thread
    // into wait mode.

    InsertDependency(entry.queue, thread_id);
    entry.cv.Wait(
        lock, _timeout,
        std::bind(&GenericMutex::DeadlockCheck, this, record_id, thread_id),
        std::bind(&GenericMutex::StopWaiting, this, record_id, thread_id));

    // Check if the request was denied. Happens on deadlock discovery.
    if (entry.queue.GetLockRequest(thread_id).IsDenied()) {
      // Permform cleanup by removing all the dependencies existing in the
      // dependency graph for the thread. Note that all the dependent/depended
      // requests of the denied request will exist only in the current queue. We
      // dont need to check queues associated with the other record identifiers.
      RemoveDependency(entry.queue, thread_id);
      // Remove the lock request from the queue
      entry.queue.RemoveLockRequest(thread_id);

      return false;
    }

    return true;
  }

  /**
   * @brief Unlock an already acquired lock on a record with the given
   * identifier.
   *
   * @param record_id Constant reference to the record identifier.
   * @param thread_id Constant reference to the thread identifier. Default set
   * to `std::this_thread::get_id()`.
   */
  void Unlock(const RecordIdType& record_id,
              const ThreadIdType& thread_id = std::this_thread::get_id()) {
    UniqueLock lock(_latch);

    // Check if an entry exists in the lock table for the given record
    // identifier.
    auto table_it = _table.find(record_id);
    if (table_it == _table.end()) {
      return;
    }

    auto& entry = table_it->second;
    // Check if a granted lock request exists in the queue.
    if (entry.queue.LockRequestExists(thread_id)) {
      if (entry.queue.GetGroupId(thread_id) == entry.granted_group_id) {
        // Remove all dependencies for the given thread identifier.
        RemoveDependency(entry.queue, thread_id);
        // Remove the lock request from the queue
        entry.queue.RemoveLockRequest(thread_id);
        // Check if no more lock requests pending
        if (entry.queue.Empty()) {
          // We can remove the entry from the lock table since the request queue
          // is empty.
          _table.erase(record_id);
        } else {
          // The request queue is not empty so we now check if all the granted
          // locks have been unlocked. If so, we can wakeup all the waiting
          // threads associated with the request queue.
          auto& front_group_id = entry.queue.Begin()->key;
          if (front_group_id != entry.granted_group_id) {
            entry.granted_group_id = front_group_id;

            // Reduces mutex contention for improved performance.
            lock.unlock();

            // TODO: [OPTIMIZATION] Insted of notifying all waiting threads,
            // design an approach to notify only the threads associated with the
            // granted request group. This reduces unnecessary thread wakeups
            // which in turn reduces mutex contention.

            entry.cv.NotifyAll();
          } else {
            // Some of the granted lock requests are still not unlocked so do
            // nothing.
          }
        }
      }
    }
  }

 private:
  /**
   * @brief Insert dependency for the given thread identifier requesting lock
   * for a record associated with the given request queue.
   *
   * @note This method is idempotent so its safe to make duplicate calls.
   *
   * @param queue Constant reference to the lock request queue.
   * @param thread_id Constant reference to the thread identifier.
   */
  void InsertDependency(const LockRequestQueue& queue,
                        const ThreadIdType& thread_id) {
    auto group_id = queue.GetGroupId(thread_id);
    auto group_it = queue.Begin();

    // Iterate through the queue till just before the given threads request
    // group. The request of the given thread is dependent on all requests in
    // these groups.
    while (group_it->key != group_id) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Add` method is idempotent. This implies that duplicate
        // calls to the method will have the desired null effect.
        _dependency_graph.Add(thread_id, request_it->key);
      }
      ++group_it;
    }

    // Iterate through the queue starting right after the given threads request
    // group till the end of the queue. All the requests in these groups are
    // dependnet on the request of the given thread.
    ++group_it;
    while (group_it != queue.End()) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Add` method is idempotent. This implies that duplicate
        // calls to the method will have the desired null effect.
        _dependency_graph.Add(request_it->key, thread_id);
      }
    }
  }

  /**
   * @brief Remove dependency for the given thread identifier having a lock
   * request for a record associated with the given request queue.
   *
   * @note This method removes dependencies only if they exist. Thus it is safe
   * to make duplicate calls.
   *
   * @param queue Constant reference to the lock request queue.
   * @param thread_id Constant reference to the thread identifier.
   */
  void RemoveDependency(const LockRequestQueue& queue,
                        const ThreadIdType& thread_id) {
    auto group_id = queue.GetGroupId(thread_id);
    auto group_it = queue.Begin();

    // Iterate through the queue till just before the given threads request
    // group. The request of the given thread is dependent on all requests in
    // these groups.
    while (group_it->key != group_id) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Remove` method only removes dependency if it exist. Thus
        // duplicate calls to the method has the desired null effect.
        _dependency_graph.Remove(thread_id, request_it->key);
      }
    }

    // Iterate through the queue starting right after the given threads request
    // group till the end of the queue. All the requests in these groups are
    // dependnet on the request of the given thread.
    ++group_it;
    while (group_it != queue.End()) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Remove` method only removes dependency if it exist. Thus
        // duplicate calls to the method has the desired null effect.
        _dependency_graph.Remove(request_it->key, thread_id);
      }
    }
  }

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
                     const ThreadIdType& thread_id) {
    // Check if the request associated with the given thread identifier is
    // denied. In that case there is no need to run the deadlock check and we
    // can simply return. This avoids unnecessary deadlock checks.
    if (_table.at(record_id).entry.queue.GetLockRequest(thread_id).IsDenied()) {
      return;
    }

    // Instantiates recovery policy
    Policy policy;

    // Search for presence of deadlock
    if (_dependency_graph.DetectCycle(thread_id, policy)) {
      auto& _thread_id = policy.Get();

      // Deny the waiting request of `_thread_id` identifier. Note that even
      // though multiple granted requests from the thread can exist in the lock
      // table, there can only be a single waiting request. We need to find and
      // deny that request here.

      // TODO: [OPTIMIZATION] Create a sperate class called LockTable and expose
      // an API method `GetWaitingRequest(const ThreadIdType& thread_id)` which
      // implements O(1) request retrival.

      // Iterate through each entry in thr lock table
      for (auto& element : _table) {
        auto& entry = element->second;
        // Check if the entry contains the waiting lock request for the above
        // thread identifier.
        if (entry.queue.LockRequestExists(_thread_id)) {
          if (entry.queue.GetGroupId(_thread_id) != entry.granted_group_id) {
            // Deny the lock request and notify all the waiting threads in the
            // queue

            // TODO: [OPTIMIZATION] Design an approach to just wakeup the thread
            // associated with the denied request. This avoids unnecessary
            // wakeup of other threads.

            entry.queue.GetLockRequest(_thread_id).Deny();
            entry.cv.NotifyAll();

            // Since only a single waiting request from the thread can ever
            // exist in the lock table, we can now simply terminate the loop.
            break;
          }
        }
      }
    }
  }

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

#endif /* GENERIC_LOCK__GENERIC_MUTEX_HPP */
