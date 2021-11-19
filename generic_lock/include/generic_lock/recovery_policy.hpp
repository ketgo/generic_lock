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

#ifndef GENERIC_LOCK__RECOVERY_POLICY_HPP
#define GENERIC_LOCK__RECOVERY_POLICY_HPP

#include <set>

namespace gl {

/**
 * @brief This policy selects the thread with the largest identifier from the
 * set of cyclic dependent threads.
 *
 * @note This policy assumes that the thread identifier type supports `<`
 * operator.
 *
 * @tparam ThreadIdType The thread identifier type.
 */
template <class ThreadIdType>
class SelectMaxPolicy {
 public:
  /**
   * @brief The method invokes the policy by selecting the identifier with max
   * value from the given set.
   *
   * @param thread_ids Reference to the set of thread identifiers.
   */
  void operator()(std::set<ThreadIdType>& thread_ids) {
    _it = thread_ids.begin();
    for (auto it = thread_ids.begin(); it != thread_ids.end(); ++it) {
      if (*_it < *it) {
        _it = it;
      }
    }
  }

  /**
   * @brief Get the thread identifier selected by the policy.
   *
   * @returns Constant reference to the policy selected thread identifier.
   */
  const ThreadIdType& Get() const { return *_it; }

 private:
  typename std::set<ThreadIdType>::iterator _it;
};

}  // namespace gl

#endif /* GENERIC_LOCK__RECOVERY_POLICY_HPP */
