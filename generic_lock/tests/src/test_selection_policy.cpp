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

/**
 * @brief Uint Test Selection Policies
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/selection_policy.hpp>

using namespace gl;

TEST(SelectionPolicyTestFixture, SelectMaxPolicy) {
  SelectMaxPolicy<size_t> policy;
  std::set<size_t> thread_ids = {1, 5, 2, 15, 7, 3, 11};

  ASSERT_EQ(policy(thread_ids), 15);
}
