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
 * @brief Uint Test Lock Request Queue
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/details/lock_request_queue.hpp>

using namespace gl::details;

class LockRequestQueueTestFixture : public ::testing::Test {
 protected:
  enum class LockMode { READ, WRITE };
  const ContentionMatrix<2> contention_matrix = {
      {{{false, true}}, {{true, true}}}};

  LockRequestQueue<LockMode, 2, size_t> queue = {contention_matrix};

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(LockRequestQueueTestFixture, TestEmplaceRequest) {
  // Emplace request into an empty queue
  auto result = queue.EmplaceLockRequest(LockMode::READ, 1);
  ASSERT_EQ(result, queue.null_group_id + 1);

  // Another request from the same thread
  ASSERT_EQ(queue.EmplaceLockRequest(LockMode::WRITE, 1), queue.null_group_id);

  // Emplace request in agreement with the last group
  ASSERT_EQ(queue.EmplaceLockRequest(LockMode::READ, 2), result);

  // Emplace request in contention with the last group
  ASSERT_EQ(queue.EmplaceLockRequest(LockMode::WRITE, 3), result + 1);
}

TEST_F(LockRequestQueueTestFixture, TestEmplaceGetRequest) {
  // Emplace request into an empty queue
  queue.EmplaceLockRequest(LockMode::READ, 1);

  auto& request = queue.GetLockRequest(1);
  ASSERT_EQ(request.GetMode(), LockMode::READ);
  ASSERT_FALSE(request.IsDenied());

  queue.GetLockRequest(1).Deny();
  ASSERT_TRUE(request.IsDenied());
}

TEST_F(LockRequestQueueTestFixture, TestEmplaceRequestGetGroupId) {
  // Emplace request into an empty queue
  auto result_1 = queue.EmplaceLockRequest(LockMode::READ, 1);
  // Emplace request in agreement with the last group
  auto result_2 = queue.EmplaceLockRequest(LockMode::READ, 2);

  ASSERT_EQ(result_1, queue.GetGroupId(1));
  ASSERT_EQ(result_2, queue.GetGroupId(2));
}

TEST_F(LockRequestQueueTestFixture, TestEmplaceRemoveGetRequest) {
  // Emplace request into an empty queue
  auto result = queue.EmplaceLockRequest(LockMode::READ, 1);

  ASSERT_EQ(result, queue.GetGroupId(1));
  queue.RemoveLockRequest(1);
  ASSERT_THROW(queue.GetGroupId(1), std::out_of_range);
  ASSERT_THROW(queue.GetLockRequest(1), std::out_of_range);
  ASSERT_EQ(queue.Size(), 0);
}
