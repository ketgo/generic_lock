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

#ifndef OP_LOG_HPP
#define OP_LOG_HPP

#include <list>
#include <mutex>

/**
 * @brief A thread-safe operation log containing sequential records of
 * operations performed on a shared data type by multiple threads.
 *
 * Note that the iterator API of the container is not thread safe.
 *
 * @tparam T Operation record type.
 */
template <class T>
class OpLog {
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

#endif /* OP_LOG_HPP */
