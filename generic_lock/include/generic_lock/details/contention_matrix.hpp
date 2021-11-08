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

#ifndef GENERIC_LOCK__DETAILS__CONTENTION_MATRIX_HPP
#define GENERIC_LOCK__DETAILS__CONTENTION_MATRIX_HPP

#include <unordered_map>

namespace gl {
namespace details {

/**
 * @brief Contention matrix is a 2-D matrix containing either `true` or `false`
 * as values. Each value represents contention between two lock modes. For
 * example, consider lock modes READ and WRITE for a read-write lock. The
 * contention matrix for the lock will be:
 *
 * @example
 * enum class LockMode { READ, WRITE };
 * const ContentionMatrix<LockMode> contention_matrix = {
 *  {LockMode::READ, {{LockMode::READ, false}, {LockMode::WRITE, true}}},
 *  {LockMode::WRITE, {{LockMode::READ, true}, {LockMode::WRITE, true}}}
 * };
 *
 * @tparam LockModeType The type of lock mode.
 */
template <class LockModeType>
using ContentionMatrix =
    std::unordered_map<LockModeType, std::unordered_map<LockModeType, bool>>;

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__CONTENTION_MATRIX_HPP */
