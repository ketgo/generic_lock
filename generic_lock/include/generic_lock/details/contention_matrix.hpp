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

#include <array>

namespace gl {
namespace details {

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
using ContentionMatrix = std::array<std::array<bool, modes_count>, modes_count>;

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__CONTENTION_MATRIX_HPP */
