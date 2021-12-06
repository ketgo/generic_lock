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

#include <generic_lock/details/condition_variable.hpp>
#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/dependency_graph.hpp>
#include <generic_lock/details/lock_request_queue.hpp>
#include <generic_lock/selection_policy.hpp>
#include <mutex>
#include <unordered_map>

namespace gl {

/**
 * @brief Contention matrix is a 2-D matrix containing either `true` or `false`
 * as values. Each value represents contention between two lock modes. For
 * example, consider lock modes READ and WRITE for a read-write lock. The
 * contention matrix for the lock will be:
 *
 * @example
 * enum class LockMode { READ, WRITE=1 };
 * const ContentionMatrix<2> contention_matrix = {{
 *  // LockMode::READ, LockMode::WRITE
 *  {{false, true}}, // LockMode::READ
 *  {{true, true}}  // LockMode::WRITE
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
 * concurrent transactions. The lock expects each data record to be uniquely
 * identified by an identifier. This can be as simple as the key of a value in a
 * hash map, or the position of a value inside a vector or array. It is left to
 * the user to define the identifier. The type of the record identifier can be
 * specified through the template parameter `RecordId`. Similarly, each
 * transaction is also uniquely identifier using its identifier. The data type
 * of the identifier can be specified using the `TransactionId` template
 * parameter. The template parameter can be simply set to `std::thread::id` when
 * each transaction correspond to an individual thread.
 *
 * Unlike the standard mutex, the generic mutex can detect and recover
 * from deadlocks between transactions. The recovery policy dictates which
 * transaction will be unblocked for recovery, and its lock request denied. The
 * type of policy can be specified through the `SelectionPolicy` template
 * parameter. By default the transaction with the maximum identifier is
 * selected.
 *
 * @note The record and transaction identifiers, along with the lock mode should
 * be hashable types.
 *
 * @todo Pass hash function object for the identifiers and lock mode.
 *
 * @tparam RecordId The record identifier type.
 * @tparam TransactionId The transaction identifier type.
 * @tparam LockMode The lock mode type.
 * @tparam modes_count The number of lock modes.
 * @tparam timeout The time in milliseconds to wait before checking for
 * deadlock. Default set to `300`.
 * @tparam SelectionPolicy Deadlock recovery selection policy type. Default set
 * to `SelectMaxPolicy<TransactionId>`.
 */
template <class RecordId, class TransactionId, class LockMode,
          size_t modes_count, size_t timeout = 300,
          class SelectionPolicy = SelectMaxPolicy<TransactionId>>
class GenericMutex {
  // Lock request queue type
  typedef details::LockRequestQueue<TransactionId, LockMode, modes_count>
      LockRequestQueue;
  // Lock request group identifier type
  typedef typename LockRequestQueue::LockRequestGroupId LockRequestGroupId;

  // Lock table entry containing queue of lock requests, a condition variable
  // for synchronizing concurrent access, and the currently granted
  // request group identifier.
  struct LockTableEntry {
    // Granted group identifier starts with value of `1` since the first
    // group in the request queue has an identifier of `1`.
    LockTableEntry(const ContentionMatrix<modes_count>& contention_matrix)
        : queue(contention_matrix), cv(), granted_group_id(1) {}

    LockRequestQueue queue;
    details::ConditionVariable cv;
    LockRequestGroupId granted_group_id;
  };

  // Table containing lock requests for different records. Each record is
  // associated with its own request queue via its unique key.
  typedef std::unordered_map<RecordId, LockTableEntry> LockTable;

  // Lock type.
  typedef std::unique_lock<std::mutex> UniqueLock;

 public:
  // Mutex traits
  typedef RecordId record_id_t;
  typedef TransactionId transaction_id_t;
  typedef LockMode lock_mode_t;

  /**
   * @brief Construct a new Generic Mutex object
   *
   * @param contention_matrix Constant reference to the contention matrix.
   */
  GenericMutex(const ContentionMatrix<modes_count>& contention_matrix)
      : contention_matrix_(contention_matrix) {}

  // Mutex not copyable
  GenericMutex(const GenericMutex& other) = delete;
  // Mutex not copy assignable
  GenericMutex& operator=(const GenericMutex& other) = delete;

  /**
   * @brief Acquire a lock on a record with the given identifier. The calling
   * transaction is blocked till the lock is successfully acquired or till the
   * request is denied due to deadlock discovery.
   *
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   * @returns `true` if the lock is successfully acquired, otherwise `false`.
   */
  bool Lock(const RecordId& record_id, const TransactionId& transaction_id,
            const LockMode& mode) {
    UniqueLock lock(latch_);

    // Creates a lock table entry if it does not exist already
    auto& entry = table_.emplace(record_id, contention_matrix_).first->second;

    // Emplace request in the queue of the record identifier
    auto& group_id = entry.queue.EmplaceLockRequest(transaction_id, mode);
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
    // group. This implies that the transaction needs to wait till the request
    // can be granted. Furthermore, the transaction is dependent on the prior
    // requests to be granted. We thus have to update the dependency graph and
    // put the transaction into wait mode.
    InsertDependency(entry.queue, transaction_id);
    entry.cv.Wait(
        lock, timeout_,
        std::bind(&GenericMutex::DeadlockCheck, this, record_id,
                  transaction_id),
        std::bind(&GenericMutex::StopWaiting, this, record_id, transaction_id));

    // Check if the request was denied. Happens on deadlock discovery.
    if (entry.queue.GetLockRequest(transaction_id).IsDenied()) {
      // Permform cleanup by removing all the dependencies existing in the
      // dependency graph for the transaction. Note that all the
      // dependent/depended requests of the denied request will exist only in
      // the current queue. We dont need to check queues associated with the
      // other record identifiers.
      RemoveDependency(entry.queue, transaction_id);
      // Remove the lock request from the queue
      entry.queue.RemoveLockRequest(transaction_id);

      return false;
    }

    return true;
  }

  /**
   * @brief Unlock an already acquired lock on a record with the given
   * identifier.
   *
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void Unlock(const RecordId& record_id, const TransactionId& transaction_id) {
    UniqueLock lock(latch_);

    // Check if an entry exists in the lock table for the given record
    // identifier.
    auto table_it = table_.find(record_id);
    if (table_it == table_.end()) {
      return;
    }

    auto& entry = table_it->second;
    // Check if a granted lock request exists in the queue.
    if (entry.queue.LockRequestExists(transaction_id)) {
      if (entry.queue.GetGroupId(transaction_id) == entry.granted_group_id) {
        // Remove all dependencies for the given transaction identifier.
        RemoveDependency(entry.queue, transaction_id);
        // Remove the lock request from the queue
        entry.queue.RemoveLockRequest(transaction_id);
        // Check if no more lock requests pending
        if (entry.queue.Empty()) {
          // We can remove the entry from the lock table since the request queue
          // is empty.
          table_.erase(record_id);
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
            // granted request group. This reduces unnecessary transaction
            // wakeups which in turn reduces mutex contention.

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
   * @brief Insert dependency for the given transaction identifier requesting
   * lock for a record associated with the given request queue.
   *
   * @note This method is idempotent so its safe to make duplicate calls.
   *
   * @param queue Constant reference to the lock request queue.
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void InsertDependency(const LockRequestQueue& queue,
                        const TransactionId& transaction_id) {
    auto group_id = queue.GetGroupId(transaction_id);
    auto group_it = queue.Begin();

    // Iterate through the queue till just before the given threads request
    // group. The request of the given transaction is dependent on all requests
    // in these groups.
    while (group_it->key != group_id) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Add` method is idempotent. This implies that duplicate
        // calls to the method will have the desired null effect.
        dependency_graph_.Add(transaction_id, request_it->key);
      }
      ++group_it;
    }

    // Iterate through the queue starting right after the given threads request
    // group till the end of the queue. All the requests in these groups are
    // dependent on the request of the given transaction.
    ++group_it;
    while (group_it != queue.End()) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Add` method is idempotent. This implies that duplicate
        // calls to the method will have the desired null effect.
        dependency_graph_.Add(request_it->key, transaction_id);
      }
      ++group_it;
    }
  }

  /**
   * @brief Remove dependency for the given transaction identifier having a lock
   * request for a record associated with the given request queue.
   *
   * @note This method removes dependencies only if they exist. Thus it is safe
   * to make duplicate calls.
   *
   * @param queue Constant reference to the lock request queue.
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void RemoveDependency(const LockRequestQueue& queue,
                        const TransactionId& transaction_id) {
    auto group_id = queue.GetGroupId(transaction_id);
    auto group_it = queue.Begin();

    // Iterate through the queue till just before the given threads request
    // group. The request of the given transaction is dependent on all requests
    // in these groups.
    while (group_it->key != group_id) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Remove` method only removes dependency if it exist. Thus
        // duplicate calls to the method has the desired null effect.
        dependency_graph_.Remove(transaction_id, request_it->key);
      }
      ++group_it;
    }

    // Iterate through the queue starting right after the given threads request
    // group till the end of the queue. All the requests in these groups are
    // dependnet on the request of the given transaction.
    ++group_it;
    while (group_it != queue.End()) {
      for (auto request_it = group_it->value.Begin();
           request_it != group_it->value.End(); ++request_it) {
        // NOTE: The `Remove` method only removes dependency if it exist. Thus
        // duplicate calls to the method has the desired null effect.
        dependency_graph_.Remove(request_it->key, transaction_id);
      }
      ++group_it;
    }
  }

  /**
   * @brief Method to check if a transaction should stop waiting. A transaction
   * can stop waiting if its lock request is granted or if the request is denied
   * due to a deadlock discovery.
   *
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns `true` if transaction can stop wating else `false`.
   */
  bool StopWaiting(const RecordId& record_id,
                   const TransactionId& transaction_id) const {
    auto& entry = table_.at(record_id);
    // Stop waiting if the request is granted or it is denied due to discovery
    // of a deadlock.
    return (entry.queue.GetGroupId(transaction_id) == entry.granted_group_id) ||
           entry.queue.GetLockRequest(transaction_id).IsDenied();
  }

  /**
   * @brief Check for presence of a deadlock and perform recovery actions if it
   * exists.
   *
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void DeadlockCheck(const RecordId& record_id,
                     const TransactionId& transaction_id) {
    // Check if the request associated with the given transaction identifier is
    // denied. In that case there is no need to run the deadlock check and
    // we can simply return. This avoids unnecessary deadlock checks.
    if (table_.at(record_id).queue.GetLockRequest(transaction_id).IsDenied()) {
      return;
    }

    // Search for presence of deadlock
    auto cycle = dependency_graph_.DetectCycle(transaction_id);
    if (!cycle.empty()) {
      // Instantiates selection policy for deadlock recovery
      SelectionPolicy policy;
      // Select transaction identifer to deny request for deadlock recovery
      auto _thread_id = policy(cycle);

      // Deny the waiting request of `_thread_id` identifier. Note that even
      // though multiple granted requests from the transaction can exist in the
      // lock table, there can only be a single waiting request. We need to find
      // and deny that request here.

      // TODO: [OPTIMIZATION] Create a sperate class called LockTable and expose
      // an API method `GetWaitingRequest(const TransactionId& transaction_id)`
      // which implements O(1) request retrival.

      // Iterate through each entry in the lock table
      for (auto& element : table_) {
        auto& entry = element.second;
        // Check if the entry contains the waiting lock request for the above
        // transaction identifier.
        if (entry.queue.LockRequestExists(_thread_id)) {
          if (entry.queue.GetGroupId(_thread_id) != entry.granted_group_id) {
            // Deny the lock request and notify all the waiting threads in the
            // queue

            // TODO: [OPTIMIZATION] Design an approach to just wakeup the
            // transaction associated with the denied request. This avoids
            // unnecessary wakeup of other threads.

            entry.queue.GetLockRequest(_thread_id).Deny();
            entry.cv.NotifyAll();

            // Since only a single waiting request from the transaction can ever
            // exist in the lock table, we can now simply terminate the loop.
            break;
          }
        }
      }
    }
  }

  // Lock mode contention matrix
  const ContentionMatrix<modes_count> contention_matrix_;
  // transaction wait timeout to check for deadlocks.
  static constexpr std::chrono::milliseconds timeout_{timeout};
  // Latch for atomic modification of the lock.
  std::mutex latch_;
  // Lock table recording state of the lock.
  LockTable table_;
  // Dependency graph between different lock requests.
  details::DependencyGraph<TransactionId> dependency_graph_;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_MUTEX_HPP */
