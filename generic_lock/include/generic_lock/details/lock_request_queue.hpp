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

#ifndef GENERIC_LOCK__DETAILS__LOCK_REQUEST_QUEUE_HPP
#define GENERIC_LOCK__DETAILS__LOCK_REQUEST_QUEUE_HPP

#include <cassert>
#include <generic_lock/details/lock_request.hpp>
#include <generic_lock/details/lock_request_group.hpp>

namespace gl {
namespace details {

/**
 * @brief A multi-indexed queue of lock requests from concurrent threads. The
 * requests are stored in the queue in chronological order. Furthermore, they
 * are grouped together such that each request in a group are in agreement with
 * each other, i.e. they can be granted simultaneously. Each request is indexed
 * on the thread identifier and its group identifier.
 *
 * @tparam LockModeType The lock mode type.
 * @tparam modes_count The number of lock modes.
 * @tparam ThreadIdType The type of thread identifier.
 */
template <class LockModeType, size_t modes_count, class ThreadIdType>
class LockRequestQueue {
 public:
  /**
   * @brief Lock request group identifier.
   *
   */
  typedef size_t LockRequestGroupId;

  /**
   * @brief Null group identifier. A group identifier starts with `1`.
   *
   */
  static constexpr LockRequestGroupId null_group_id = 0;

 private:
  typedef LockRequest<LockModeType> LockRequestType;
  typedef LockRequestGroup<LockModeType, modes_count, ThreadIdType>
      LockRequestGroupType;
  typedef IndexedList<LockRequestGroupId, LockRequestGroupType>
      RequestGroupListType;

 public:
  typedef typename RequestGroupListType::Iterator Iterator;
  typedef typename RequestGroupListType::ConstIterator ConstIterator;

  /**
   * @brief Construct a new Lock Request Queue object.
   *
   * @param contention_matrix Constant reference to the contention matrix.
   */
  LockRequestQueue(const ContentionMatrix<modes_count>& contention_matrix)
      : _contention_matrix(contention_matrix), _group_id_map(), _groups() {}

  /**
   * @brief Emplace a lock request into the queue. The request is checked for
   * agreement with the last request group in the queue. If there is agreement,
   * the request is added to the group, otherwise a new group is created. The
   * method returns the identifier of the group to which the emplaced request
   * belongs. Note that no operation is performed if a lock request for the
   * given thread already exists in the queue. In this case a null group
   * identifier is returned.
   *
   * @param mode Constant reference to the requested lock mode.
   * @param thread_id Constant reference to the thread identifier.
   * @returns Constant reference to the identifier of the group to which the
   * emplaced request belongs.
   */
  const LockRequestGroupId& EmplaceLockRequest(const LockModeType& mode,
                                               const ThreadIdType& thread_id) {
    // If no group exist in the queue then create a new group and emplace
    // the request in it
    if (_groups.Empty()) {
      return EmplaceNewRequestGroup(null_group_id + 1, mode, thread_id);
    }

    // Check that a prior request by the same thread does not exist
    if (_group_id_map.find(thread_id) != _group_id_map.end()) {
      // Prior request by the thread exists so return null group identifier.
      return null_group_id;
    }

    // No prior request exists so try to emplace the new request into the last
    // group
    auto& last_group = _groups.Back();
    if (last_group.value.EmplaceLockRequest(mode, thread_id,
                                            _contention_matrix)) {
      _group_id_map[thread_id] = last_group.key;
      return last_group.key;
    }

    // Could not emplace into the last group so create a new group and emplace
    // the request inside it
    return EmplaceNewRequestGroup(last_group.key + 1, mode, thread_id);
  }

  /**
   * @brief Get the lock request in the queue for the given thread identifier. A
   * `std::out_of_range` exception is thrown if no request exists.
   *
   * @param thread_id Constant reference to the thread identifier.
   * @returns Reference to the lock request.
   */
  LockRequestType& GetLockRequest(const ThreadIdType& thread_id) {
    auto& group_id = _group_id_map.at(thread_id);
    return _groups.At(group_id).GetLockRequest(thread_id);
  }

  /**
   * @brief Get the lock request in the queue for the given thread identifier. A
   * `std::out_of_range` exception is thrown if no request exists.
   *
   * @param thread_id Constant reference to the thread identifier.
   * @returns Constant Reference to the lock request.
   */
  const LockRequestType& GetLockRequest(const ThreadIdType& thread_id) const {
    auto& group_id = _group_id_map.at(thread_id);
    return _groups.At(group_id).GetLockRequest(thread_id);
  }

  /**
   * @brief Remove a request for the given thread identifier from the queue. A
   * `std::out_of_range` exception is thrown if no request exists.
   *
   * @param thread_id Constant reference to the thread identifier.
   */
  void RemoveLockRequest(const ThreadIdType& thread_id) {
    auto& group_id = _group_id_map.at(thread_id);
    auto& group = _groups.At(group_id);
    group.RemoveLockRequest(thread_id);
    if (group.Empty()) {
      _groups.Erase(group_id);
    }
    _group_id_map.erase(thread_id);
  }

  /**
   * @brief Check if a lock request for the given thread identifier exists in
   * the queue.
   *
   * @param thread_id Constant reference to the thread identifier.
   * @returns `true` if request is in the queue else `false`.
   */
  bool LockRequestExists(const ThreadIdType& thread_id) {
    return _group_id_map.find(thread_id) != _group_id_map.end();
  }

  /**
   * @brief Get the lock request group identifier for the given thread
   * identifier.
   *
   * @param thread_id Constant reference to the thread identifier.
   * @returns Constant reference to the lock request group identifier.
   */
  const LockRequestGroupId& GetGroupId(const ThreadIdType& thread_id) const {
    return _group_id_map.at(thread_id);
  }

  /**
   * @brief Get an iterator pointing to the begining of the container.
   *
   * @returns Iterator pointing to the begining of the container.
   */
  Iterator Begin() { return _groups.Begin(); }

  /**
   * @brief Get a constant iterator pointing to the begining of the container.
   *
   * @return Constant iterator pointing to the begining of the container.
   */
  ConstIterator Begin() const { return _groups.Begin(); }

  /**
   * @brief Get an iterator pointing to the end of the container.
   *
   * @returns Iterator pointing to the end of the container.
   */
  Iterator End() { return _groups.End(); }

  /**
   * @brief Get a constant iterator pointing to the end of the container.
   *
   * @returns Constant iterator pointing to the end of the container.
   */
  ConstIterator End() const { return _groups.End(); }

  /**
   * @brief Check if the lock request queue is empty.
   *
   * @returns `true` if empty else `false`.
   */
  bool Empty() const { return _groups.Empty(); }

  /**
   * @brief Get the number of request groups in the queue.
   *
   * @returns Number of request groups in the queue
   */
  size_t Size() const { return _groups.Size(); }

 private:
  /**
   * @brief Emplace a new request group into the queue and emplace a new lock
   * request into the newly created group.
   *
   * @param group_id Constant reference to the new group identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier.
   * @returns Constant reference to the newly created group identifier.
   */
  const LockRequestGroupId& EmplaceNewRequestGroup(
      const LockRequestGroupId& group_id, const LockModeType& mode,
      const ThreadIdType& thread_id) {
    // Creates an empty request group
    auto result = _groups.EmplaceBack(group_id);
    // Assert that we were able to create the empty group.
    assert(result.second);
    // Emplace the request into the group
    result.first->value.EmplaceLockRequest(mode, thread_id, _contention_matrix);
    // Record the mapping between the thread and the new group identifier
    _group_id_map[thread_id] = result.first->key;
    // Return the new group identifier
    return _group_id_map[thread_id];
  }

  // List of lock request groups indexed on their group identifier
  RequestGroupListType _groups;
  // Map between the thread identifiers and the associated lock request group
  // identifier.
  std::unordered_map<ThreadIdType, LockRequestGroupId> _group_id_map;
  // Contention matrix for creating lock request groups
  const ContentionMatrix<modes_count>& _contention_matrix;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_REQUEST_QUEUE_HPP */
