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

#ifndef GENERIC_LOCK__DETAILS__CONDITION_VARIABLE_HPP
#define GENERIC_LOCK__DETAILS__CONDITION_VARIABLE_HPP

#include <condition_variable>

namespace gl {
namespace details {

/**
 * Wrapper around the `std::condition_variable` class. Along with exposing all
 * the public API of `std::condition_variable`, the wrapper class also exposes
 * additional APIs.
 *
 */
class ConditionVariable {
 public:
  // Native handle of the underlying condition variable.
  typedef std::condition_variable::native_handle_type native_handle_type;

  /**
   * Construct a new ConditionVariable object.
   *
   */
  ConditionVariable() = default;

  /**
   * Wait causes the current thread to block until the condition variable is
   * notified or a spurious wakeup occurs. The method atomically unlocks `lock`,
   * blocks the current executing thread, and adds it to the list of waiting
   * threads. The thread will be unblocked when `NotifyAll()` or `NotifyOne()`
   * is executed. It may also be unblocked spuriously. When unblocked,
   * regardless of the reason, lock is reacquired.
   *
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   */
  void Wait(std::unique_lock<std::mutex>& lock) { _cv.wait(lock); }

  /**
   * Wait causes the current thread to block until the condition variable is
   * notified or a spurious wakeup occurs. The method atomically unlocks `lock`,
   * blocks the current executing thread, and adds it to the list of waiting
   * threads. The thread will be unblocked when `NotifyAll()` or `NotifyOne()`
   * is executed. It may also be unblocked spuriously. When unblocked,
   * regardless of the reason, lock is reacquired and the predicate is checked.
   * Until the predicate is satisfied (`!stop_waiting() == true`), the thread
   * waits again.
   *
   * @tparam Predicate The predicate type.
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   * @param stop_waiting The predicate which returns ​false if the waiting
   * should be continued. The signature of the predicate function should be
   * equivalent to `bool stop_waiting();`.
   */
  template <class Predicate>
  void Wait(std::unique_lock<std::mutex>& lock, Predicate stop_waiting) {
    _cv.wait(lock, std::move(stop_waiting));
  }

  /**
   * Wait causes the current thread to block until the condition variable is
   * notified or a spurious wakeup occurs. While blocking it periodically
   * unblocks the thread and executes a specified callback. The method
   * atomically unlocks `lock`, blocks the current executing thread, and adds it
   * to the list of waiting threads. The thread will be unblocked when
   * `NotifyAll()` or `NotifyOne()` is executed. It will also unblock
   * periodically after every `duration` and execute the callback. It may also
   * be unblocked spuriously. When unblocked, regardless of the reason, lock is
   * reacquired.
   *
   * @tparam Rep An arithmetic type representing the number of ticks.
   * @tparam Period A ratio representing tick period.
   * @tparam Callback The callback type.
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   * @param duration Constant reference to an object of type
   * `std::chrono::duration` representing the time interval between the
   * periodical execution of the callback. Note that the duration must be small
   * enough not to overflow when added to `std::chrono::steady_clock::now()`.
   * @param callback The callback executed periodically. The signature of the
   * callback function should be equivalent to `void callback();`.
   */
  template <class Rep, class Period, class Callback>
  void Wait(std::unique_lock<std::mutex>& lock,
            const std::chrono::duration<Rep, Period>& duration,
            Callback callback) {
    while (_cv.wait_for(lock, duration) == std::cv_status::timeout) {
      callback();
    }
  }

  /**
   * Wait causes the current thread to block until the condition variable is
   * notified or a spurious wakeup occurs. While blocking it periodically
   * unblocks the thread and executes a specified callback. The method
   * atomically unlocks `lock`, blocks the current executing thread, and adds it
   * to the list of waiting threads. The thread will be unblocked when
   * `NotifyAll()` or `NotifyOne()` is executed. It will also unblock
   * periodically after every `duration` and execute the callback. It may also
   * be unblocked spuriously. When unblocked, regardless of the reason, lock is
   * reacquired and the predicate is checked. Until the predicate is satisfied
   * (`!stop_waiting() == true`), the thread waits again.
   *
   * @tparam Rep An arithmetic type representing the number of ticks.
   * @tparam Period A ratio representing tick period.
   * @tparam Callback The callback type.
   * @tparam Predicate The predicate type.
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   * @param duration Constant reference to an object of type
   * `std::chrono::duration` representing the time interval between the
   * periodical execution of the callback. Note that the duration must be small
   * enough not to overflow when added to `std::chrono::steady_clock::now()`.
   * @param callback The callback executed periodically. The signature of the
   * callback function should be equivalent to `void callback();`.
   * @param stop_waiting The predicate which returns ​false if the waiting
   * should be continued. The signature of the predicate function should be
   * equivalent to `bool stop_waiting();`.
   */
  template <class Rep, class Period, class Callback, class Predicate>
  void Wait(std::unique_lock<std::mutex>& lock,
            const std::chrono::duration<Rep, Period>& duration,
            Callback callback, Predicate stop_waiting) {
    while (!stop_waiting()) {
      if (_cv.wait_for(lock, duration) == std::cv_status::timeout) {
        callback();
      }
    }
  }

  /**
   * WaitFor causes the current thread to block until the condition variable is
   * notified, a specified duration passes, or a spurious wakeup occurs. The
   * method atomically releases `lock`, blocks the current executing thread, and
   * adds it to the list of waiting threads. The thread will be unblocked when
   * `NotifyAll()` or `NotifyOne()` is executed, or when the relative timeout
   * `duration` expires. It may also be unblocked spuriously. When unblocked,
   * regardless of the reason, lock is reacquired.
   *
   * @tparam Rep An arithmetic type representing the number of ticks.
   * @tparam Period A ratio representing tick period.
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   * @param duration Constant reference to an object of type
   * `std::chrono::duration` representing the maximum time to spend waiting.
   * Note that the duration must be small enough not to overflow when added to
   * `std::chrono::steady_clock::now()`.
   * @returns `std::cv_status::timeout` if the relative timeout specified by
   * `duration` expired, `std::cv_status::no_timeout` otherwise.
   */
  template <class Rep, class Period>
  std::cv_status WaitFor(std::unique_lock<std::mutex>& lock,
                         const std::chrono::duration<Rep, Period>& duration) {
    return _cv.wait_for(lock, duration);
  }

  /**
   * WaitFor causes the current thread to block until the condition variable is
   * notified, a specified duration passes, or a spurious wakeup occurs. The
   * method atomically releases `lock`, blocks the current executing thread, and
   * adds it to the list of waiting threads. The thread will be unblocked when
   * `NotifyAll()` or `NotifyOne()` is executed, or when the relative timeout
   * `duration` expires. It may also be unblocked spuriously. When unblocked,
   * regardless of the reason, lock is reacquired and the predicate is checked.
   * Until the predicate is satisfied (`!stop_waiting()`
   * == true), the thread waits again.
   *
   * @tparam Rep An arithmetic type representing the number of ticks.
   * @tparam Period A ratio representing tick period.
   * @tparam Predicate The predicate type.
   * @param lock Reference to the unique lock which can be locked by the current
   * thread.
   * @param duration Constant reference to an object of type
   * `std::chrono::duration` representing the maximum time to spend waiting.
   * Note that the duration must be small enough not to overflow when added to
   * `std::chrono::steady_clock::now()`.
   * @param stop_waiting The predicate which returns ​false if the waiting
   * should be continued. The signature of the predicate function should be
   * equivalent to `bool stop_waiting();`.
   * @returns `false` if the predicate `stop_waiting` still evaluates to `false`
   * after the `duration` timeout expired, otherwise `true`.
   */
  template <class Rep, class Period, class Predicate>
  bool WaitFor(std::unique_lock<std::mutex>& lock,
               const std::chrono::duration<Rep, Period>& duration,
               Predicate stop_waiting) {
    return _cv.wait_for(lock, duration, std::move(stop_waiting));
  }

  /**
   * Unblocks all threads currently waiting on the condition variable.
   *
   */
  void NotifyAll() noexcept { _cv.notify_all(); }

  /**
   * If any threads are waiting on the condition variable, calling notify_one
   * unblocks one of the waiting threads.
   *
   */
  void NotifyOne() noexcept { _cv.notify_one(); }

  /**
   * Accesses the native handle of the condition variable. The meaning and the
   * type of the result of this function is implementation-defined. On a POSIX
   * system, this may be a value of type `pthread_cond_t*`. On a Windows system,
   * this may be a `PCONDITION_VARIABLE`.
   *
   * @returns The native handle of this condition variable.
   */
  native_handle_type NativeHandle() { return _cv.native_handle(); }

 private:
  std::condition_variable _cv;  // Native condition variable
};

}  // namespace details
}  // namespace gl

#endif /* GENERIC_LOCK__DETAILS__CONDITION_VARIABLE_HPP */
