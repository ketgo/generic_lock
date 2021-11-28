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

#include <unordered_map>

#include "op_log.hpp"

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

 public:
  /**
   * @brief The record stores the type of operation performed on a map along
   * with the identifier of the thread performing the operation. There are two
   * types of operations supported: READ and WRITE. For the READ operation, the
   * data structure stored the key and associated value read. In case of the
   * WRITE operation the key and value written are stored.
   *
   */
  struct OpRecord {
    T thread_id;
    enum class Type { READ, WRITE } type;
    K key;
    V value;

    OpRecord() = default;
    OpRecord(const T& thread_id, const Type& type, const K& key, const V& value)
        : thread_id(thread_id), type(type), key(key), value(value) {}
  };
  /**
   * @brief Operation log type.
   *
   */
  typedef OpLog<OpRecord> op_log_t;

  /**
   * @brief Construct a new Logged Map object.
   *
   */
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
    _op_log.emplace(thread_id, OpRecord::Type::READ, key, value);
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
    _op_log.emplace(thread_id, OpRecord::Type::WRITE, key, value);
  }

  /**
   * @brief Returns a constant reference to the operation log of the map.
   *
   * @returns Constant reference to the operation log.
   */
  const op_log_t& OperationLog() const { return _op_log; }

 private:
  map_t _map;
  mutable op_log_t _op_log;
};

#endif /* LOGGED_MAP_HPP */
