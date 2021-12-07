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
 * @brief Uint Test Lock Request
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/details/lock_request.hpp>

using namespace gl::details;

class LockRequestTestFixture : public ::testing::Test {
 protected:
  static constexpr size_t i_mode = 1;
  LockRequest<size_t> request = {i_mode};

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(LockRequestTestFixture, GetSetLockMode) {
  ASSERT_EQ(request.GetMode(), i_mode);
  request.SetMode(2);
  ASSERT_EQ(request.GetMode(), 2);
}

TEST_F(LockRequestTestFixture, ApproveDenyRequest) {
  ASSERT_FALSE(request.IsDenied());
  request.Deny();
  ASSERT_TRUE(request.IsDenied());
  request.Approve();
  ASSERT_FALSE(request.IsDenied());
}