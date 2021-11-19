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

#ifndef GENERIC_LOCK__GENERIC_LOCK_HPP
#define GENERIC_LOCK__GENERIC_LOCK_HPP

namespace gl {

/**
 * @brief Generic lock is a general-purpose generic mutex ownership wrapper.
 *
 * @tparam GenericMutexType The type of generic mutex.
 */
template <class GenericMutexType>
class GenericLock {
  typedef typename GenericMutexType::record_id_t RecordIdType;
  typedef typename GenericMutexType::lock_mode_t LockModeType;
  typedef typename GenericMutexType::thread_id_t ThreadIdType;

 public:
  /**
   * @brief Construct a new Generic Lock object.
   *
   */
  GenericLock()
      : _owns(false),
        _record_id(),
        _mode(),
        _thread_id(),
        _generic_mutex_ptr(nullptr),
        _denied(false) {}

  /**
   * @brief Construct a new Generic Lock object.
   *
   * @param generic_mutex Reference to the generic mutex to manage.
   */
  GenericLock(GenericMutexType& generic_mutex)
      : _owns(false),
        _record_id(),
        _mode(),
        _thread_id(),
        _generic_mutex_ptr(&generic_mutex),
        _denied(false) {}

  /**
   * @brief Construct a new Generic Lock object and acquire a lock.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier. Default set
   * to `std::this_thread::get_id()`.
   */
  GenericLock(GenericMutexType& generic_mutex, const RecordIdType& record_id,
              const LockModeType& mode,
              const ThreadIdType& thread_id = std::this_thread::get_id())
      : _owns(true),
        _record_id(record_id),
        _mode(mode),
        _thread_id(thread_id),
        _generic_mutex_ptr(&generic_mutex),
        _denied(!_generic_mutex_ptr->Lock(_record_id, _mode, _thread_id)) {}

  /**
   * @brief Destroy the Generic Lock object. The underlying mutex is unlocked if
   * locked.
   *
   */
  ~GenericLock() { Unlock(); }

  /**
   * @brief Lock the underlying generic mutex.
   *
   * @returns `true` if lock acquired else `false`.
   */
  bool Lock() {
    if (!_owns) {
      _owns = true;
      _denied = !_generic_mutex_ptr->Lock(_record_id, _mode, _thread_id);
    }
    return !_denied;
  }

  // Lock not copyable
  GenericLock(const GenericLock& other) = delete;
  // Lock not copy assignable
  GenericLock& operator=(const GenericLock& other) = delete;

  /**
   * @brief Move construct a new Generic lock object.
   *
   * @param other Rvalue reference to the other generic lock object.
   */
  GenericLock(GenericLock&& other) {}

  /**
   * @brief Move assign the generic lock.
   *
   * @param other Rvalue reference to the other generic lock object.
   * @returns Reference to the generic lock object.
   */
  GenericLock& operator=(GenericLock&& other) {}

  /**
   * @brief Unlock the underlying generic mutex.
   *
   */
  void Unlock() {
    if (_owns) {
      _owns = false;
      if (!_denied) {
        _denied = false;
        _generic_mutex_ptr->Unlock(_record_id, _thread_id);
      }
    }
  }

  // TODO: Change this to `IsAcquired`.
  /**
   * @brief Check if the lock is denied, i.e. could not be acquired.
   *
   * @return `true` if lock was denied else `false`.
   */
  bool IsDenied() const { return _denied; }

  /**
   * @brief Check if the lock owns the underlying generic mutex. A mutex is
   * owned when its `Lock` method is called. It is disowned upon unlock.
   *
   * @returns `true` if the mutex is owned else `false`.
   */
  bool OwnsLock() const { return _owns; }

  /**
   * @brief Get pointer to the underlying generic mutex.
   *
   * @returns Pointer to the underlying generic mutex.
   */
  GenericMutexType* Mutex() const { return _generic_mutex_ptr; }

  /**
   * @brief Check if the lock is owned and acquired.
   *
   * @returns `true` if the lock is owned and acquired else `false`.
   */
  explicit operator bool() const { return _owns && !_denied; }

 private:
  bool _owns;
  RecordIdType _record_id;
  LockModeType _mode;
  ThreadIdType _thread_id;
  GenericMutexType* _generic_mutex_ptr;
  bool _denied;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
