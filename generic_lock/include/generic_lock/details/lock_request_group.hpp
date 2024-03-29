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

#ifndef GENERIC_LOCK__DETAILS__LOCK_REQUEST_GROUP_HPP
#define GENERIC_LOCK__DETAILS__LOCK_REQUEST_GROUP_HPP

#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/indexed_list.hpp>
#include <generic_lock/details/lock_request.hpp>

namespace gl {
namespace details {

/**
 * Group of lock request that are in agreement with each other such that all the
 * requests in the group can be granted simultaneously.
 *
 * @tparam TransactionId Transaction identifier type.
 * @tparam LockMode Lock mode type.
 * @tparam modes_count Number of lock modes.
 */
template <class TransactionId, class LockMode, size_t modes_count>
class LockRequestGroup {
  typedef IndexedList<TransactionId, LockRequest<LockMode>> LockRequestList;

 public:
  typedef typename LockRequestList::Iterator Iterator;
  typedef typename LockRequestList::ConstIterator ConstIterator;

  /**
   * Construct a new Lock Request Group object
   *
   */
  LockRequestGroup() = default;

  /**
   * Emplace a lock request into the group if there is no contention. The
   * contention matrix is used to check for contention with any of the existing
   * requests in the group. If a contention is found then the method returns
   * `false, otherwise `true` is returned after emplacement.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   * @param contention_matrix Constant reference to the contention matrix.
   * @returns `true` on success else `false`.
   */
  bool EmplaceLockRequest(
      const TransactionId& transaction_id, const LockMode& mode,
      const ContentionMatrix<modes_count>& contention_matrix) {
    // Check for contention with all the requests in the group which are not in
    // a denied state.
    auto it = _requests.Begin();
    while (it != _requests.End()) {
      if (contention_matrix[int(it->value.GetMode())][int(mode)] &&
          !it->value.IsDenied()) {
        // Contention found with a request so return false.
        return false;
      }
      ++it;
    }
    // No contention found so emplace the request into the group
    auto result = _requests.EmplaceBack(transaction_id, mode);

    return result.second;
  }

  /**
   * Get the lock request in the group for the given transaction identifier. If
   * no such request exists then a `std::out_of_range` exception is thrown.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns Reference to the lock request.
   */
  LockRequest<LockMode>& GetLockRequest(const TransactionId& transaction_id) {
    return _requests.At(transaction_id);
  }

  /**
   * Get the lock request in the group for the given transaction identifier. If
   * no such request exists then a `std::out_of_range` exception is thrown.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   * @returns Constant reference to the lock request.
   */
  const LockRequest<LockMode>& GetLockRequest(
      const TransactionId& transaction_id) const {
    return _requests.At(transaction_id);
  }

  /**
   * Remove lock request for the given transaction identifier from the group. If
   * no request exists for the transaction identifier then a `std::out_of_range`
   * exception is thrown.
   *
   * @param transaction_id Constant reference to the transaction identifier.
   */
  void RemoveLockRequest(const TransactionId& transaction_id) {
    _requests.Erase(transaction_id);
  }

  /**
   * Get the number of requests in the group.
   *
   * @returns Number of requests in the group.
   */
  size_t Size() const { return _requests.Size(); }

  /**
   * Check if the request group is empty.
   *
   * @returns `true` if empty else `false`.
   */
  bool Empty() const { return _requests.Empty(); }

  /**
   * Get an iterator pointing to the begining of the container.
   *
   * @returns Iterator pointing to the begining of the container.
   */
  Iterator Begin() { return _requests.Begin(); }

  /**
   * Get a constant iterator pointing to the begining of the container.
   *
   * @return Constant iterator pointing to the begining of the container.
   */
  ConstIterator Begin() const { return _requests.Begin(); }

  /**
   * Get an iterator pointing to the end of the container.
   *
   * @returns Iterator pointing to the end of the container.
   */
  Iterator End() { return _requests.End(); }

  /**
   * Get a constant iterator pointing to the end of the container.
   *
   * @returns Constant iterator pointing to the end of the container.
   */
  ConstIterator End() const { return _requests.End(); }

 private:
  // Indexed list of lock requests which are part of the group.
  LockRequestList _requests;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_REQUEST_GROUP_HPP */
