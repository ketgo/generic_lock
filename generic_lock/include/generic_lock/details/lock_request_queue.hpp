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

#ifndef GENERIC_LOCK__DETAILS__LOCK_REQUEST_HPP
#define GENERIC_LOCK__DETAILS__LOCK_REQUEST_HPP

#include <cassert>

#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/indexed_list.hpp>

namespace gl {
namespace details {

/**
 * @brief A multi-indexed queue of lock requests from concurrent threads. The
 * requests are stored in the queue in chronological order. Furthermore, they
 * are grouped together such that each request in a group are in agreement with
 * each other, i.e. they can be granted simultaneously. Each request is indexed
 * on the thread identifier and its group identifier.
 *
 * @tparam LockModeType The type of lock mode.
 * @tparam ThreadIdType The type of thread identifier.
 */
template <class LockModeType, class ThreadIdType>
class LockRequestQueue {
 public:
  /**
   * @brief Lock request data structure contining information of the request
   * made by a thread for acquiring a lock on a data record.
   *
   */
  struct LockRequest {
    /**
     * @brief Construct a new Lock Request object.
     *
     * @param mode Constant reference to the lock mode.
     */
    LockRequest(const LockModeType& mode) : mode(mode), denied(false) {}

    // The lock mode requested.
    LockModeType mode;
    // Flag indicating if the request should be denied. This is set to true if
    // the request causes a deadlock.
    bool denied;
  };

  /**
   * @brief Group of lock request that are in agreement with each other such
   * that all the requests in the group can be granted simultaneously.
   *
   */
  class LockRequestGroup {
   public:
    /**
     * @brief Construct a new Lock Request Group object
     *
     */
    LockRequestGroup() = default;

    /**
     * @brief Emplace a lock request into the group if there is no contention.
     * The contention matrix is used to check for contention with any of the
     * existing requests in the group. If the request is found to be not in
     * agreement with the group then the methed return `false, otherwise `true`
     * is returned after emplacement.
     *
     * @param mode Constant reference to the lock mode.
     * @param thread_id Constant reference to the thread identifier.
     * @param contention_matrix Constant reference to the contention matrix.
     * @returns `true` on success else `false`.
     */
    bool EmplaceBack(const LockModeType& mode, const ThreadIdType& thread_id,
                     const ContentionMatrix<LockModeType>& contention_matrix) {
      // Check for contention with all the member sof the group
      auto it = requests.Begin();
      while (it != requests.End()) {
        if (contention_matrix[it->value.mode][mode]) {
          // Contention found with a request so return false.
          return false;
        }
        ++it;
      }
      // No contention found so emplace the request into the group
      auto result = requests.EmplaceBack(thread_id, mode);
      return result.second;
    }

   private:
    // Indexed list of lock requests which are part of the group.
    IndexedList<ThreadIdType, LockRequest> requests;
  };

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

  /**
   * @brief Construct a new Lock Request Queue object.
   *
   * @param contention_matrix Constant reference to the contention matrix.
   */
  LockRequestQueue(const ContentionMatrix<LockModeType>& contention_matrix)
      : _contention_matrix(contention_matrix), _group_map(), _groups() {}

  /**
   * @brief Emplace a lock request into the queue. The request is checked for
   * agreement with the last request group in the queue. If there is agreement,
   * the request is added to the group, otherwise a new group is created. The
   * method returns the identifier of the group to which the emplaced request
   * belongs. Note that no operation is performed if a lock request for the
   * given thread already exists in the queue. In this case the group identifier
   * for the existing request is returned.
   *
   * @param mode Constant reference to the requested lock mode.
   * @param thread_id Constant reference to the thread identifier.
   * @returns Constant reference to the identifier of the group to which the
   * emplaced request belongs.
   */
  const LockRequestGroupId& EmplaceRequest(const LockModeType& mode,
                                           const ThreadIdType& thread_id) {
    // If no group exist in the queue then create a new group and emplace
    // the request in it
    if (_groups.Empty()) {
      // Creates an empty request group
      auto result = _groups.EmplaceBack(null_group_id + 1);
      // Assert that we were able to create the empty group.
      assert(result.second);
      // Emplace the request into the group
      result.first->value.EmplaceBack(mode, thread_id, _contention_matrix);
      // Record the mapping between the thread and the new group identifier
      _group_map[thread_id] = result.first->key;
      // Return the new group identifier
      return result.first->key;
    }

    // Check that a prior request by the same thread does not exist
    auto it = _group_map.find(thread_id);
    if (it != _group_map.end()) {
      // Prior request by the thread exists so return its group identifier.
      return it->first;
    }

    // No prior request exists so try to emplace the new request into the last
    // group
    auto& last_group = _groups.Back();
    if (last_group.value.EmplaceBack(mode, thread_id, _contention_matrix)) {
      return last_group.key;
    }

    // Could not emplace into the last group so create a new group and emplace
    // the request in it
    // Creates an empty request group
    auto result = _groups.EmplaceBack(last_group.key + 1);
    // Assert that we were able to create the empty group.
    assert(result.second);
    // Emplace the request into the group
    result.first->value.EmplaceBack(mode, thread_id, _contention_matrix);
    // Record the mapping between the thread and the new group identifier
    _group_map[thread_id] = result.first->key;
    // Return the new group identifier
    return result.first->key;
  }

  LockRequest& Request(const ThreadIdType& thread_id) {}
  const LockRequest& Request(const ThreadIdType& thread_id) const {}

  bool RemoveRequest(const ThreadIdType& thread_id) {}
  void DenyRequest(const ThreadIdType& thread_id) {}
  bool RequestDenied(const ThreadIdType& thread_id) const {}

  LockRequestGroupId& RequestGroupId(const ThreadIdType& thread_id) {}
  const LockRequestGroupId& RequestGroupId(
      const ThreadIdType& thread_id) const {}

  LockRequestGroup& RequestGroup(const LockRequestGroupId& group_id) {}
  const LockRequestGroup& RequestGroup(
      const LockRequestGroupId& group_id) const {}

 private:
  // List of lock request groups indexed on their group identifier
  IndexedList<LockRequestGroupId, LockRequestGroup> _groups;
  // Map between the thread identifiers and the associated lock request group
  // identifier.
  std::unordered_map<ThreadIdType, LockRequestGroupId> _group_map;
  // Contention matrix for creating lock request groups
  const ContentionMatrix<LockModeType>& _contention_matrix;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_REQUEST_HPP */
