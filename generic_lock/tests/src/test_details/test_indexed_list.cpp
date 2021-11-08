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
 * @brief Uint Test Indxed List
 *
 */

#include <gtest/gtest.h>

#include <string>
#include <generic_lock/details/indexed_list.hpp>

using namespace gl::details;

class IndexedListTestFixture : public ::testing::Test {
 protected:
  struct Node {
    Node() = default;
    Node(const std::string& str, const float& num) : str(str), num(num) {}

    bool operator==(const Node& other) const {
      return str == other.str && num == other.num;
    }
    bool operator!=(const Node& other) const {
      return !(this->operator==(other));
    }

    std::string str;
    float num;
  };

  IndexedList<int, Node> list;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(IndexedListTestFixture, TestEmplaceBackAt) {
  ASSERT_EQ(list.Size(), 0);
  ASSERT_TRUE(list.Empty());

  list.EmplaceBack(1, "1.0", 1.0);
  list.EmplaceBack(2, "2.0", 2.0);

  ASSERT_EQ(list.Size(), 2);
  ASSERT_FALSE(list.Empty());
  ASSERT_EQ(list.At(1), Node("1.0", 1.0));
  ASSERT_EQ(list.At(2), Node("2.0", 2.0));
}

TEST_F(IndexedListTestFixture, TestAtNonexistingKey) {
  ASSERT_THROW(list.At(1), std::out_of_range);
}

TEST_F(IndexedListTestFixture, TestEmplaceBackDuplicateKey) {
  list.EmplaceBack(1, "1.0", 1.0);

  auto emplaced = list.EmplaceBack(1, "2.0", 2.0);
  ASSERT_FALSE(emplaced.second);
  ASSERT_EQ(emplaced.first->value, Node("1.0", 1.0));
}

TEST_F(IndexedListTestFixture, TestEmplaceBackFind) {
  list.EmplaceBack(1, "1.0", 1.0);
  list.EmplaceBack(2, "2.0", 2.0);

  auto it = list.Find(1);
  ASSERT_EQ(it, list.Begin());
  ASSERT_EQ(it->value, Node("1.0", 1.0));
  ASSERT_EQ((++it)->value, Node("2.0", 2.0));
  ASSERT_EQ(++it, list.End());

  it = list.Find(2);
  ASSERT_NE(it, list.Begin());
  ASSERT_EQ(it->value, Node("2.0", 2.0));
  ASSERT_EQ(++it, list.End());
}

TEST_F(IndexedListTestFixture, TestEmplaceBackErase) {
  list.EmplaceBack(1, "1.0", 1.0);
  list.EmplaceBack(2, "2.0", 2.0);

  auto it = list.Erase(1);
  ASSERT_THROW(list.At(1), std::out_of_range);
  ASSERT_EQ(it->key, 2);
  ASSERT_EQ(it->value, Node("2.0", 2.0));

  it = list.Erase(it);
  ASSERT_EQ(it, list.End());
  ASSERT_TRUE(list.Empty());
}

TEST_F(IndexedListTestFixture, TestEraseNonexistingKey) {
  ASSERT_THROW(list.Erase(1), std::out_of_range);
}
