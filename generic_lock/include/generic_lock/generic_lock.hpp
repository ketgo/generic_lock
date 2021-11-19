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
        _generic_mutex_ptr(nullptr) {}

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
        _generic_mutex_ptr(&generic_mutex) {}

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
        _generic_mutex_ptr(&generic_mutex) {
    _generic_mutex_ptr->Lock();
  }

  bool Lock() {}
  bool Unlock() {}
  bool IsDenied() const {}
  bool OwnsLock() const {}
  GenericMutexType* Mutex() const {}
  explicit operator bool() const {}

 private:
  /**
   * @brief Take ownership by locking the mutex.
   *
   */
  void Own() {}

  /**
   * @brief Release ownership of the lock.
   *
   */
  void Release() {}

  bool _owns;
  RecordIdType _record_id;
  LockModeType _mode;
  ThreadIdType _thread_id;
  GenericMutexType* _generic_mutex_ptr;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
