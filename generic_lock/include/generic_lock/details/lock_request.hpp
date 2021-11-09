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

namespace gl {
namespace details {

/**
 * @brief Lock request contains the type of lock mode requested and if the
 * request is denied due to deadlock discorvery.
 *
 * @tparam LockModeType The lock mode type.
 */
template <class LockModeType>
class LockRequest {
 public:
  /**
   * @brief Construct a new Lock Request object.
   *
   * @param mode Constant reference to the lock mode.
   */
  LockRequest(const LockModeType& mode) : _mode(mode), _denied(false) {}

  /**
   * @brief Get the requested lock mode.
   *
   * @returns Constant reference to the lock mode.
   */
  const LockModeType& GetMode() const { return _mode; }

  /**
   * @brief set the requested lock mode.
   *
   * @returns Constant reference to the lock mode.
   */
  void SetMode(const LockModeType& mode) { _mode = mode; }

  /**
   * @brief Set the lock request as denied.
   *
   */
  void Deny() { _denied = true; }

  /**
   * @brief Set the lock request as approved.
   *
   */
  void Approve() { _denied = false; }

  /**
   * @brief Check if the lock request is denied.
   *
   * @returns `true` if denied else `false`.
   */
  bool IsDenied() const { return _denied; }

 private:
  // The lock mode requested.
  LockModeType _mode;
  // Flag indicating if the request should be denied. This is set to true if
  // the request causes a deadlock.
  bool _denied;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__LOCK_REQUEST_HPP */
