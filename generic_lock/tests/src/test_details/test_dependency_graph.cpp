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
 * @brief Uint Test Dependency Graph
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/details/dependency_graph.hpp>

using namespace gl::details;

class DependencyGraphTestFixture : public ::testing::Test {
 protected:
  DependencyGraph<size_t> graph;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DependencyGraphTestFixture, TestAddRemoveEdge) {
  graph.Add(1, 2);
  ASSERT_TRUE(graph.IsDependent(1, 2));

  graph.Add(2, 3);
  ASSERT_TRUE(graph.IsDependent(2, 3));

  graph.Add(4, 1);
  ASSERT_TRUE(graph.IsDependent(4, 1));

  graph.Remove(1, 2);
  ASSERT_FALSE(graph.IsDependent(1, 2));

  graph.Remove(2, 3);
  ASSERT_FALSE(graph.IsDependent(2, 3));

  graph.Remove(4, 1);
  ASSERT_FALSE(graph.IsDependent(4, 1));
}

TEST_F(DependencyGraphTestFixture, TestDetectCycleExists) {
  std::set<size_t> cycle = {2, 5, 6, 7};

  graph.Add(1, 2);
  graph.Add(2, 3);
  graph.Add(3, 4);
  graph.Add(2, 5);
  graph.Add(5, 4);
  graph.Add(5, 6);
  graph.Add(6, 7);
  graph.Add(7, 2);
  graph.Add(6, 8);
  graph.Add(8, 9);
  graph.Add(8, 10);

  graph.DetectCycle(4,
                    [&](std::set<size_t>& ids) { ASSERT_TRUE(ids.empty()); });
  graph.DetectCycle(1, [&](std::set<size_t>& ids) { ASSERT_EQ(ids, cycle); });
  graph.DetectCycle(2, [&](std::set<size_t>& ids) { ASSERT_EQ(ids, cycle); });
  graph.DetectCycle(5, [&](std::set<size_t>& ids) { ASSERT_EQ(ids, cycle); });
  graph.DetectCycle(6, [&](std::set<size_t>& ids) { ASSERT_EQ(ids, cycle); });
  graph.DetectCycle(7, [&](std::set<size_t>& ids) { ASSERT_EQ(ids, cycle); });
}

TEST_F(DependencyGraphTestFixture, TestDetectNoCycle) {
  graph.Add(1, 2);
  graph.Add(2, 3);
  graph.Add(3, 4);
  graph.Add(2, 5);
  graph.Add(5, 4);
  graph.Add(5, 6);
  graph.Add(6, 7);
  graph.Add(6, 8);
  graph.Add(8, 9);
  graph.Add(8, 10);

  graph.DetectCycle(1,
                    [&](std::set<size_t>& ids) { ASSERT_TRUE(ids.empty()); });
}