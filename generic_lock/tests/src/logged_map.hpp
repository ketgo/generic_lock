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

#ifndef LOGGED_MAP_HPP
#define LOGGED_MAP_HPP

#include <list>
#include <unordered_map>
#include <mutex>

namespace {

/**
 * @brief A thread-safe operation log containing sequential records of
 * operations performed on a shared map by multiple threads.
 *
 * Note that the iterator API of the container is not thread safe.
 *
 * @tparam T Operation record type.
 */
template <class T>
class Log {
  typedef std::list<T> queue_t;

 public:
  typedef typename queue_t::iterator iterator_t;
  typedef typename queue_t::const_iterator const_iterator_t;

  template <class... Args>
  void emplace(Args&&... args) {
    std::lock_guard guard(_mutex);
    _list.emplace_back(std::forward<Args>(args)...);
  }

  void push(const T& value) {
    std::lock_guard guard(_mutex);
    _list.push_back(value);
  }

  void pop() {
    std::lock_guard guard(_mutex);
    _list.pop_front();
  }

  T front() {
    std::lock_guard guard(_mutex);
    return _list.front();
  }

  const T front() const {
    std::lock_guard guard(_mutex);
    return _list.front();
  }

  T back() {
    std::lock_guard guard(_mutex);
    return _list.back();
  }

  const T back() const {
    std::lock_guard guard(_mutex);
    return _list.back();
  }

  iterator_t begin() {
    std::lock_guard guard(_mutex);
    return _list.begin();
  }

  const_iterator_t begin() const {
    std::lock_guard guard(_mutex);
    return _list.begin();
  }

  iterator_t end() {
    std::lock_guard guard(_mutex);
    return _list.end();
  }

  const_iterator_t end() const {
    std::lock_guard guard(_mutex);
    return _list.end();
  }

 private:
  mutable std::mutex _mutex;
  queue_t _list;
};

/**
 * @brief The record stores the type of operation performed on a map along with
 * the identifier of the thread performing the operation. There are two types of
 * operations supported: READ and WRITE. For the READ operation, the data
 * structure stored the key and associated value read. In case of the WRITE
 * operation the key and value written are stored.
 *
 * @tparam T The thread identifier type.
 * @tparam K The type of key.
 * @tparam V The type of value.
 */
template <class T, class K, class V>
struct Operation {
  T thread_id;
  enum class Type { READ, WRITE } type;
  K key;
  V value;

  Operation() = default;
  Operation(const T& thread_id, const Type& type, const K& key, const V& value)
      : thread_id(thread_id), type(type), key(key), value(value) {}
};

}  // namespace

/**
 * @brief A map container with all its succesfully performed operations logged.
 * Note that only the operations log used by the map is thread safe. The map
 * itself is not thread safe, Data races and invalid memory access can occure
 * when concurrent writes happen for the same key, or when re-hashing happens
 * for increasing the capacity of the container. This makes the map appropriate
 * for testing syncronization premitives.
 *
 * @tparam T The thread identifier type.
 * @tparam K The key type.
 * @tparam V The value type.
 */
template <class T, class K, class V>
class LoggedMap {
  typedef std::unordered_map<K, V> map_t;
  typedef ::Operation<T, K, V> op_record_t;
  typedef ::Log<op_record_t> op_log_t;

 public:
  template <class... Args>
  LoggedMap(Args&&... args) : _map(std::forward<Args>(args)...), _op_log() {}
  LoggedMap(std::initializer_list<typename map_t::value_type> li)
      : _map(li), _op_log() {}

  /**
   * @brief Get the value associated with the given key. The method throws
   * `std::out_of_range` exception if the key is not found in the map.
   *
   * @param thread_id Constant reference to the identifier of the thread calling
   * the method.
   * @param key Constant reference to the key.
   * @returns Constant reference to the value assocaited with the key.
   */
  const V& Get(const T& thread_id, const K& key) const {
    auto& value = _map.at(key);
    _op_log.emplace(thread_id, op_record_t::Type::READ, key, value);
    return value;
  }

  /**
   * @brief Set a key-value pair into the map. If the key-value pair already
   * exists then its value is updated.
   *
   * @param thread_id Constant reference to the identifier of the thread calling
   * the method.
   * @param key Constant reference to the key.
   * @param value Constant reference to the value.
   */
  void Set(const T& thread_id, const K& key, const V& value) {
    auto result = _map.insert({key, value});
    if (!result.second) {
      result.first->second = value;
    }
    _op_log.emplace(thread_id, op_record_t::Type::WRITE, key, value);
  }

  /**
   * @brief Returns a constant reference to the operation log of the map.
   *
   * @returns Constant reference to the operation log.
   */
  const op_log_t& OpLog() const { return _op_log; }

 private:
  map_t _map;
  mutable op_log_t _op_log;
};

#endif /* LOGGED_MAP_HPP */
