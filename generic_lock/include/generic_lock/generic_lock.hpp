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
#include <unordered_map>

#include <generic_lock/details/dependency_graph.hpp>
#include <generic_lock/details/condition_variable.hpp>
#include <generic_lock/details/contention_matrix.hpp>

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
 */
template <class RecordIdType, class LockModeType, size_t modes_count,
          class ThreadIdType = std::thread::id>
class GenericLock {
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
   * @brief
   *
   * @param record_id
   * @param mode
   * @param thread_id
   * @return true
   * @return false
   */
  bool Lock(const RecordIdType& record_id, const LockModeType& mode,
            const ThreadIdType& thread_id = std::this_thread::get_id());

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
   * @brief
   *
   * @param record_id
   * @param thread_id
   * @return true
   * @return false
   */
  bool Unlock(const RecordIdType& record_id,
              const ThreadIdType& thread_id = std::this_thread::get_id());

 private:
  const ContentionMatrix<modes_count> _contention_matrix;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
