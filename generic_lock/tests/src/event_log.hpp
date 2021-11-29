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

#ifndef EVENT_LOG_HPP
#define EVENT_LOG_HPP

#include <list>
#include <mutex>

/**
 * @brief A thread-safe event log containing sequential records of
 * events puahed by multiple threads.
 *
 * Note that the iterator API of the container is not thread safe.
 *
 * @tparam T Event type.
 */
template <class T>
class EventLog {
  typedef std::list<T> queue_t;

 public:
  typedef typename queue_t::iterator iterator_t;
  typedef typename queue_t::const_iterator const_iterator_t;

  /**
   * @brief Eamplace an event into the log.
   *
   * @tparam Args The constructor argument types of the event.
   * @param args Rvalue reference to the constructor arguments of the event.
   */
  template <class... Args>
  void emplace(Args&&... args) {
    std::lock_guard guard(_mutex);
    _list.emplace_back(std::forward<Args>(args)...);
  }

  /**
   * @brief Push an event into the log.
   *
   * @param event Constant reference to the event.
   */
  void push(const T& event) {
    std::lock_guard guard(_mutex);
    _list.push_back(event);
  }

  /**
   * @brief Remove the oldest event from the log.
   *
   */
  void pop() {
    std::lock_guard guard(_mutex);
    _list.pop_front();
  }

  /**
   * @brief Get the latest event in the log.
   *
   * @returns The value of the latest event.
   */
  T front() {
    std::lock_guard guard(_mutex);
    return _list.front();
  }

  /**
   * @brief Get the latest event in the log.
   *
   * @returns The constant value of the latest event.
   */
  const T front() const {
    std::lock_guard guard(_mutex);
    return _list.front();
  }

  /**
   * @brief Get the oldest event in the log.
   *
   * @returns The value of the oldest event.
   */
  T back() {
    std::lock_guard guard(_mutex);
    return _list.back();
  }

  /**
   * @brief Get the oldest event in the log.
   *
   * @returns The constant value of the oldest event.
   */
  const T back() const {
    std::lock_guard guard(_mutex);
    return _list.back();
  }

  /**
   * @brief Get the iterator pointing to the begining of the log.
   *
   * @returns Iterator pointing to the begining of the log.
   */
  iterator_t begin() {
    std::lock_guard guard(_mutex);
    return _list.begin();
  }

  /**
   * @brief Get the constant iterator pointing to the begining of the log.
   *
   * @returns Constant iterator pointing to the begining of the log.
   */
  const_iterator_t begin() const {
    std::lock_guard guard(_mutex);
    return _list.begin();
  }

  /**
   * @brief Get the iterator pointing to one past the last event in the log.
   *
   * @returns Iterator pointing to to one past the last event in the log.
   */
  iterator_t end() {
    std::lock_guard guard(_mutex);
    return _list.end();
  }

  /**
   * @brief Get the constant iterator pointing to one past the last event in the
   * log.
   *
   * @returns Constant iterator pointing to to one past the last event in the
   * log.
   */
  const_iterator_t end() const {
    std::lock_guard guard(_mutex);
    return _list.end();
  }

  /**
   * @brief Returns the number of events in the log.
   *
   * @returns The number of events in the log.
   */
  size_t size() const {
    std::lock_guard guard(_mutex);
    return _list.size();
  }

 private:
  mutable std::mutex _mutex;
  queue_t _list;
};

#endif /* EVENT_LOG_HPP */
