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
 * @brief Uint Test Lock Request Group
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/details/lock_request_group.hpp>

using namespace gl::details;

class LockRequestGroupTestFixture : public ::testing::Test {
 protected:
  typedef size_t ThreadId;
  enum class LockMode { READ, WRITE };
  const ContentionMatrix<2> contention_matrix = {
      {{{false, true}}, {{true, true}}}};

  LockRequestGroup<LockMode, 2, ThreadId> group;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(LockRequestGroupTestFixture, EmplaceGetRequest) {
  // Emplace request into an empty group
  ASSERT_TRUE(group.EmplaceRequest(LockMode::READ, 1, contention_matrix));

  // Another request from the same thread
  ASSERT_FALSE(group.EmplaceRequest(LockMode::READ, 1, contention_matrix));

  // Emplace request in agreement with the last group
  ASSERT_TRUE(group.EmplaceRequest(LockMode::READ, 2, contention_matrix));

  // Emplace request in contention with the last group
  ASSERT_FALSE(group.EmplaceRequest(LockMode::WRITE, 3, contention_matrix));

  ASSERT_EQ(group.Size(), 2);
  ASSERT_EQ(group.GetLockRequest(1).mode, LockMode::READ);
  ASSERT_FALSE(group.GetLockRequest(1).IsDenied());

  group.GetLockRequest(1).Deny();
  ASSERT_TRUE(group.GetLockRequest(1).IsDenied());
}