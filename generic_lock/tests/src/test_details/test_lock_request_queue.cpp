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

  LockRequestQueue<2, size_t> queue = {contention_matrix};

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(LockRequestQueueTestFixture, EmplaceRequest) {
  queue.EmplaceRequest(int(LockMode::READ), 1);
}