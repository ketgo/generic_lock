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
 * A multi-indexed queue of lock requests from concurrent transactions. The
 * requests are stored in the queue in chronological order. Furthermore, they
 * are grouped together such that each request in a group are in agreement with
 * each other, i.e. they can be granted simultaneously. Each request is indexed
 * on the transaction identifier and its group identifier.
 *
 *
 * @tparam TransactionId Transaction identifier type.
 * @tparam LockMode Lock mode type.
 * @tparam modes_count Number of lock modes.
 */
template <class TransactionId, class LockMode, size_t modes_count>
class LockRequestQueue {
 public:
  /**
   * Lock request group identifier.
   *
   */
  typedef size_t LockRequestGroupId;

  /**
   * Null group identifier. A group identifier starts with `1`.
   *
   */
  static constexpr LockRequestGroupId null_group_id = 0;

 private:
  typedef LockRequestGroup<TransactionId, LockMode, modes_count>
      LockRequestGroupType;
  typedef IndexedList<LockRequestGroupId, LockRequestGroupType>
      RequestGroupListType;

 public:
  typedef typename RequestGroupListType::Iterator Iterator;
  typedef typename RequestGroupListType::ConstIterator ConstIterator;

  /**
   * Emplace a lock request into the queue. The request is checked for agreement
   * with the last request group in the queue. If there is agreement, the
   * request is added to the group, otherwise a new group is created. The method
   * returns the identifier of the group to which the emplaced request belongs.
   * Note that no operation is performed if a lock request for the given
   * transaction already exists in the queue. In this case a null group
   * identifier is returned.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the requested lock mode.
   * @param contention_matrix Constant reference to the contention matrix.
   * @returns Constant reference to the identifier of the group to which the
   * emplaced request belongs.
   */
  const LockRequestGroupId& EmplaceLockRequest(
      const TransactionId& transaction_id, const LockMode& mode,
      const ContentionMatrix<modes_count>& contention_matrix) {
    // If no group exist in the queue then create a new group and emplace
    // the request in it
    if (groups_.Empty()) {
      return EmplaceNewRequestGroup(null_group_id + 1, transaction_id, mode,
                                    contention_matrix);
    }

    // Check that a prior request by the same transaction does not exist
    if (group_id_map_.find(transaction_id) != group_id_map_.end()) {
      // Prior request by the transaction exists so return null group
      // identifier.
      return null_group_id;
    }

    // No prior request exists so try to emplace the new request into the last
    // group
    auto& last_group = groups_.Back();
    if (last_group.value.EmplaceLockRequest(transaction_id, mode,
                                            contention_matrix)) {
      group_id_map_[transaction_id] = last_group.key;
      return last_group.key;
    }

    // Could not emplace into the last group so create a new group and emplace
    // the request inside it
    return EmplaceNewRequestGroup(last_group.key + 1, transaction_id, mode,
                                  contention_matrix);
  }

  /**
   * Get the lock request in the queue for the given transaction identifier. A
   * `std::out_of_range` exception is thrown if no request exists.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns Reference to the lock request.
   */
  LockRequest<LockMode>& GetLockRequest(const TransactionId& transaction_id) {
    auto& group_id = group_id_map_.at(transaction_id);
    return groups_.At(group_id).GetLockRequest(transaction_id);
  }

  /**
   * Get the lock request in the queue for the given transaction
   * identifier. A `std::out_of_range` exception is thrown if no request exists.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns Constant Reference to the lock request.
   */
  const LockRequest<LockMode>& GetLockRequest(
      const TransactionId& transaction_id) const {
    auto& group_id = group_id_map_.at(transaction_id);
    return groups_.At(group_id).GetLockRequest(transaction_id);
  }

  /**
   * Remove a request for the given transaction identifier from the queue. A
   * `std::out_of_range` exception is thrown if no request exists.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void RemoveLockRequest(const TransactionId& transaction_id) {
    auto& group_id = group_id_map_.at(transaction_id);
    auto& group = groups_.At(group_id);
    group.RemoveLockRequest(transaction_id);
    if (group.Empty()) {
      groups_.Erase(group_id);
    }
    group_id_map_.erase(transaction_id);
  }

  /**
   * Check if a lock request for the given transaction identifier exists in the
   * queue.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns `true` if request is in the queue else `false`.
   */
  bool LockRequestExists(const TransactionId& transaction_id) {
    return group_id_map_.find(transaction_id) != group_id_map_.end();
  }

  /**
   * Get the lock request group identifier for the given transaction identifier.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns Constant reference to the lock request group identifier.
   */
  const LockRequestGroupId& GetGroupId(
      const TransactionId& transaction_id) const {
    return group_id_map_.at(transaction_id);
  }

  /**
   * Get an iterator pointing to the begining of the container.
   *
   * @returns Iterator pointing to the begining of the container.
   */
  Iterator Begin() { return groups_.Begin(); }

  /**
   * Get a constant iterator pointing to the begining of the container.
   *
   * @return Constant iterator pointing to the begining of the container.
   */
  ConstIterator Begin() const { return groups_.Begin(); }

  /**
   * Get an iterator pointing to the end of the container.
   *
   * @returns Iterator pointing to the end of the container.
   */
  Iterator End() { return groups_.End(); }

  /**
   * Get a constant iterator pointing to the end of the container.
   *
   * @returns Constant iterator pointing to the end of the container.
   */
  ConstIterator End() const { return groups_.End(); }

  /**
   * Check if the lock request queue is empty.
   *
   * @returns `true` if empty else `false`.
   */
  bool Empty() const { return groups_.Empty(); }

  /**
   * Get the number of request groups in the queue.
   *
   * @returns Number of request groups in the queue
   */
  size_t Size() const { return groups_.Size(); }

 private:
  /**
   * Emplace a new request group into the queue and emplace a new lock request
   * into the newly created group.
   *
   * @param group_id Constant reference to the new group identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   * @param contention_matrix Constant reference to the contention matrix.
   * @returns Constant reference to the newly created group identifier.
   */
  const LockRequestGroupId& EmplaceNewRequestGroup(
      const LockRequestGroupId& group_id, const TransactionId& transaction_id,
      const LockMode& mode,
      const ContentionMatrix<modes_count>& contention_matrix) {
    // Creates an empty request group
    auto result = groups_.EmplaceBack(group_id);
    // Assert that we were able to create the empty group.
    assert(result.second);
    // Emplace the request into the group
    result.first->value.EmplaceLockRequest(transaction_id, mode,
                                           contention_matrix);
    // Record the mapping between the transaction and the new group identifier
    group_id_map_[transaction_id] = result.first->key;
    // Return the new group identifier
    return group_id_map_[transaction_id];
  }

  // List of lock request groups indexed on their group identifier
  RequestGroupListType groups_;
  // Map between the transaction identifiers and the associated lock request
  // group identifier.
  std::unordered_map<TransactionId, LockRequestGroupId> group_id_map_;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_REQUEST_QUEUE_HPP */
