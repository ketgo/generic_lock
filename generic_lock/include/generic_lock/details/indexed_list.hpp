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

#ifndef GENERIC_LOCK__DETAILS__INDEXED_LIST_HPP
#define GENERIC_LOCK__DETAILS__INDEXED_LIST_HPP

#include <list>
#include <unordered_map>

namespace gl {
namespace details {

/**
 * @brief An indexed list is a container containing list of values indexed on a
 * key. Thus each value can be accessed in O(1) using its key. The values are
 * stored in the order they are inserted, unlike `std::map` where they are
 * ordered by key.
 *
 * @tparam KeyType The type of key.
 * @tparam ValueType The type of value.
 */
template <class KeyType, class ValueType>
class IndexedList {
 public:
  struct Node {
    template <class... Args>
    Node(const KeyType& key, Args&&... args)
        : key(key), value(std::forward<Args>(args)...) {}

    KeyType key;
    ValueType value;
  };

  typedef std::list<Node> List;
  typedef typename List::iterator Iterator;
  typedef typename List::const_iterator ConstIterator;
  typedef std::unordered_map<KeyType, Iterator> Index;

  /**
   * @brief Construct a new Indexed List object
   *
   */
  IndexedList() = default;

  /**
   * @brief Inserts a new element at the end of the list, right after its
   * current last element. This new element is constructed in place using args
   * as the arguments for its construction.
   *
   * @tparam Args The type of arguments forwarded to construct the new element.
   * @param key Index key of the element.
   * @param args Arguments forwarded to construct the new element.
   * @returns A pair consisting of an iterator to the inserted element
   * (or to the element that prevented the insertion) and a bool denoting
   * whether the insertion took place.
   */
  template <class... Args>
  std::pair<Iterator, bool> EmplaceBack(const KeyType& key, Args&&... args) {
    auto index_it = _index.find(key);
    if (index_it != _index.end()) return {index_it->second, false};

    _list.emplace_back(key, std::forward<Args>(args)...);
    auto list_it = std::prev(_list.end());
    _index.emplace(key, list_it);

    return {list_it, true};
  }

  /**
   * @brief Get the value for the given key. This is an O(1) operation. Note
   * that an `std::out_of_range` exception is thrown if the given key-value pair
   * does not exist in the container.
   *
   * @param key Constant reference to the key.
   * @returns Reference to the associated value.
   */
  ValueType& At(const KeyType& key) { return _index.at(key)->value; }

  /**
   * @brief Get the value for the given key. This is an O(1) operation. Note
   * that an `std::out_of_range` exception is thrown if the given key-value pair
   * does not exist in the container.
   *
   * @param key Constant reference to the key.
   * @returns Constant reference to the associated value.
   */
  const ValueType& At(const KeyType& key) const {
    return _index.at(key)->value;
  }

  /**
   * @brief Get the key-value pair at the begining of the list.
   *
   * @returns Reference to the front node.
   */
  Node& Front() { return _list.front().value; }

  /**
   * @brief Get the key-value pair at the begining of the list.
   *
   * @returns Constant reference to the front node.
   */
  const Node& Front() const { return _list.front().value; }

  /**
   * @brief Get the key-value pair at the end of the list.
   *
   * @returns Reference to the end node.
   */
  Node& Back() { return _list.back().value; }

  /**
   * @brief Get the key-value pair at the end of the list.
   *
   * @returns Constant reference to the end node.
   */
  const Node& Back() const { return _list.back().value; }

  /**
   * @brief Find the value associated with the given key. This is an O(1)
   * operation.
   *
   * @param key Constant reference to the key.
   * @returns Iterator to the node containing the requested key-value pair.
   */
  Iterator Find(const KeyType& key) {
    auto index_it = _index.find(key);
    if (index_it == _index.end()) return _list.end();
    return index_it->second;
  }

  /**
   * @brief Find the value associated with the given key. This is an O(1)
   * operation.
   *
   * @param key Constant reference to the key.
   * @returns Constant iterator to the node containing the requested key-value
   * pair.
   */
  ConstIterator Find(const KeyType& key) const {
    auto index_it = _index.find(key);
    if (index_it == _index.end()) return _list.end();
    return index_it->second;
  }

  /**
   * @brief Erase the key-value pair for the given key.
   *
   * @param key Constant reference to the key.
   * @returns Iterator to the node right after the erased node.
   */
  Iterator Erase(const KeyType& key) {
    auto pos = _index.at(key);
    _index.erase(key);
    return _list.erase(pos);
  }

  /**
   * @brief Erase the key-value pair at the given iterator position.
   *
   * @param pos Iterator pointing to the node to erase.
   * @returns Iterator to the node right after the erased node.
   */
  Iterator Erase(Iterator pos) {
    _index.erase(pos->key);
    return _list.erase(pos);
  }

  /**
   * @brief Erase the key-value pair at the given iterator position.
   *
   * @param pos Constant iterator pointing to the node to erase.
   * @returns Iterator to the node right after the erased node.
   */
  Iterator Erase(ConstIterator pos) {
    _index.erase(pos->key);
    return _list.erase(pos);
  }

  /**
   * @brief Get an iterator pointing to the begining of the container.
   *
   * @returns Iterator pointing to the begining of the container.
   */
  Iterator Begin() { return _list.begin(); }

  /**
   * @brief Get a constant iterator pointing to the begining of the container.
   *
   * @return Constant iterator pointing to the begining of the container.
   */
  ConstIterator Begin() const { return _list.begin(); }

  /**
   * @brief Get an iterator pointing to the end of the container.
   *
   * @returns Iterator pointing to the end of the container.
   */
  Iterator End() { return _list.end(); }

  /**
   * @brief Get a constant iterator pointing to the end of the container.
   *
   * @returns Constant iterator pointing to the end of the container.
   */
  ConstIterator End() const { return _list.end(); }

  /**
   * @brief Get the number of elements in the container.
   *
   * @returns Number of elements in the container.
   */
  size_t Size() const { return _list.size(); }

  /**
   * @brief Check if the container is empty.
   *
   * @returns `true` if empty else `false`.
   */
  bool Empty() const { return _list.empty(); }

 private:
  List _list;
  Index _index;
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__INDEXED_LIST_HPP */
