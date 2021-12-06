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

#ifndef GENERIC_LOCK__DETAILS__LOCK_TABLE_HPP
#define GENERIC_LOCK__DETAILS__LOCK_TABLE_HPP

#include <generic_lock/details/contention_matrix.hpp>
#include <generic_lock/details/lock_request.hpp>

namespace gl {
namespace details {

/**
 * Lock table contains information on the granted and waiting lock requests on
 * different records. It maps each
 *
 * @tparam RecordId Record identifier type.
 * @tparam TransactionId Transaction identifier type.
 * @tparam modes_count Number of lock modes.
 */
template <class RecordId, class TransactionId, size_t modes_count>
class LockTable {
 public:
  /**
   * Construct a new Lock Table object.
   *
   * @param contention_matrix Constant reference to the contention matrix.
   */
  explicit LockTable(const ContentionMatrix<modes_count>& contention_matrix)
      : contention_matrix_(contention_matrix) {}

  LockRequest& WaitingLockRequest(const TransactionId& transaction_id) {}

 private:
  const ContentionMatrix<modes_count> contention_matrix_;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_TABLE_HPP */
