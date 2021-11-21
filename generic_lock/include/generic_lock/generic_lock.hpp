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

#include <system_error>

namespace gl {

/**
 * @brief Defer lock tag used for specifing constructor behaviour of generic
 * lock. If the defer lock tag is passed, the generic lock will not lock its
 * underlying mutex during construction.
 *
 */
struct DeferLockTag {
  explicit DeferLockTag() = default;
};
constexpr DeferLockTag DeferLock = DeferLockTag();

/**
 * @brief Adopt lock tag used for specifing constructor behaviour of generic
 * lock. If the adopt lock tag is passed, the generic lock will assume the
 * caller already holds lock to its underlying mutex during construction.
 *
 */
struct AdoptLockTag {
  explicit AdoptLockTag() = default;
};
constexpr AdoptLockTag AdoptLock = AdoptLockTag();

/**
 * @brief Generic lock is a general-purpose generic mutex ownership wrapper. The
 * lock has three possible states:
 *
 * None: The mutex is not owned by the lock since no attempt to acquire the lock
 * has been performed.
 *
 * Owned: The mutex is owned by the lock and in turn implies that the mutex is
 * locked.
 *
 * Denied: The lock on the mutex was denied in order to prevent or recover from
 * a deadlock. This in turn implies that the mutex is not owned.
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
      : _record_id(),
        _mode(),
        _thread_id(),
        _generic_mutex_ptr(nullptr),
        _owns(false),
        _denied(false) {}

  /**
   * @brief Construct a new Generic Lock object and acquire a lock on the mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier.
   */
  GenericLock(GenericMutexType& generic_mutex, const RecordIdType& record_id,
              const LockModeType& mode, const ThreadIdType& thread_id)
      : _record_id(record_id),
        _mode(mode),
        _thread_id(thread_id),
        _generic_mutex_ptr(&generic_mutex),
        _owns(_generic_mutex_ptr->Lock(_record_id, _mode, _thread_id)),
        _denied(!_owns) {}

  /**
   * @brief Construct a new Generic Lock object without acquiring a lock on the
   * mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier.
   */
  GenericLock(GenericMutexType& generic_mutex, const RecordIdType& record_id,
              const LockModeType& mode, const ThreadIdType& thread_id,
              DeferLockTag)
      : _record_id(record_id),
        _mode(mode),
        _thread_id(thread_id),
        _generic_mutex_ptr(&generic_mutex),
        _owns(false),
        _denied(false) {}

  /**
   * @brief Construct a new Generic Lock object assuming the caller already
   * acquired a lock on the mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param mode Constant reference to the lock mode.
   * @param thread_id Constant reference to the thread identifier.
   */
  GenericLock(GenericMutexType& generic_mutex, const RecordIdType& record_id,
              const LockModeType& mode, const ThreadIdType& thread_id,
              AdoptLockTag)
      : _record_id(record_id),
        _mode(mode),
        _thread_id(thread_id),
        _generic_mutex_ptr(&generic_mutex),
        _owns(true),
        _denied(false) {}

  // Lock not copyable
  GenericLock(const GenericLock& other) = delete;

  /**
   * @brief Move construct a new Generic lock object.
   *
   * @param other Rvalue reference to the other generic lock object.
   */
  GenericLock(GenericLock&& other)
      : _record_id(other._record_id),
        _mode(other._mode),
        _thread_id(other._thread_id),
        _generic_mutex_ptr(other._generic_mutex_ptr),
        _owns(other._owns),
        _denied(other._denied) {
    other._owns = false;
    other._denied = false;
    other._generic_mutex_ptr = nullptr;
  }

  /**
   * @brief Destroy the Generic Lock object. The underlying mutex is unlocked if
   * locked.
   *
   */
  ~GenericLock() {
    if (_owns) {
      _generic_mutex_ptr->Unlock(_record_id, _thread_id);
    }
  }

  /**
   * @brief Lock the underlying generic mutex.
   *
   * @returns `true` if the lock is acquired else `false`.
   */
  bool Lock() {
    if (_generic_mutex_ptr == nullptr) {
      throw std::system_error(EPERM, std::system_category(),
                              "GenericLock::Lock: references null mutex");
    }
    if (_owns) {
      throw std::system_error(EDEADLK, std::system_category(),
                              "GenericLock::Lock: already locked");
    }
    _owns = _generic_mutex_ptr->Lock(_record_id, _mode, _thread_id);
    _denied = !_owns;
    return _owns;
  }

  /**
   * @brief Unlock the underlying generic mutex.
   *
   */
  void Unlock() {
    if (!_owns) {
      throw std::system_error(EPERM, std::system_category(),
                              "GenericLock::UnLock: not locked");
    }
    _owns = false;  // NOTE: No need to set _denied as it will be already set to
                    // `false` since _owns is set to `true`.
    _generic_mutex_ptr->Unlock(_record_id, _thread_id);
  }

  /**
   * @brief Releases ownership of the associated generic mutex without
   * unlocking. If a lock is held prior to this call, the caller is now
   * responsible to unlock the mutex.
   *
   * @returns Pointer to the associated generic mutex or a null pointer.
   */
  GenericMutexType* Release() {
    GenericMutexType* rvalue = _generic_mutex_ptr;
    _generic_mutex_ptr = nullptr;
    _owns = false;
    _denied = false;
    return rvalue;
  }

  /**
   * @brief Check if the lock on the underlying mutex has been denied. Note that
   * the mutex is not considered owned if the lock on it is denied.
   *
   * @returns `true` if lock is denied else `false`.
   */
  bool IsDenied() const { return _denied; }

  /**
   * @brief Check if the underlying generic mutex is owned. A mutex is owned
   * when its locked, and disowned upon unlock. Note that the mutex might get
   * failed to be owned if the lock on it is denied.
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
   * @brief Get the identifier of the record associated with the lock.
   *
   * @returns Constant reference to the record identifier.
   */
  const RecordIdType& RecordId() const { return _record_id; }

  /**
   * @brief Get the lock mode.
   *
   * @returns Constant reference to the lock mode.
   */
  const LockModeType& LockMode() const { return _mode; }

  /**
   * @brief Get the identifier of the thread associated with the lock.
   *
   * @returns Constant reference to the thread identifier.
   */
  const ThreadIdType& ThreadId() const { return _thread_id; }

  /**
   * @brief Check if the lock is owned.
   *
   * @returns `true` if the lock is owned and acquired else `false`.
   */
  explicit operator bool() const { return _owns; }

  // Lock not copy assignable
  GenericLock& operator=(const GenericLock& other) = delete;

  /**
   * @brief Move assign the generic lock.
   *
   * @param other Rvalue reference to the other generic lock object.
   * @returns Reference to the generic lock object.
   */
  GenericLock& operator=(GenericLock&& other) {
    if (*this != &other) {
      if (_owns) {
        _generic_mutex_ptr->Unlock(_record_id, _thread_id);
      }
      _record_id = other._record_id;
      _mode = other._mode;
      _thread_id = other._thread_id;
      _generic_mutex_ptr = other._generic_mutex_ptr;
      _owns = other._owns;
      _denied = other._denied;
      other._generic_mutex_ptr = nullptr;
      other._owns = false;
      other._denied = false;
    }
    return *this;
  }

 private:
  RecordIdType _record_id;
  LockModeType _mode;
  ThreadIdType _thread_id;
  GenericMutexType* _generic_mutex_ptr;
  bool _owns;
  bool _denied;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
