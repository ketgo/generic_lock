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
 * @tparam GenericMutex The type of generic mutex.
 */
template <class GenericMutex>
class GenericLock {
  typedef typename GenericMutex::record_id_t record_id_t;
  typedef typename GenericMutex::lock_mode_t lock_mode_t;
  typedef typename GenericMutex::transaction_id_t transaction_id_t;

 public:
  /**
   * @brief Construct a new Generic Lock object.
   *
   */
  GenericLock()
      : record_id_(),
        mode_(),
        transaction_id_(),
        generic_mutex_ptr_(nullptr),
        owns_(false),
        denied_(false) {}

  /**
   * @brief Construct a new Generic Lock object and acquire a lock on the mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   */
  GenericLock(GenericMutex& generic_mutex, const record_id_t& record_id,
              const transaction_id_t& transaction_id, const lock_mode_t& mode)
      : record_id_(record_id),
        transaction_id_(transaction_id),
        mode_(mode),
        generic_mutex_ptr_(&generic_mutex),
        owns_(generic_mutex_ptr_->Lock(record_id_, transaction_id_, mode_)),
        denied_(!owns_) {}

  /**
   * @brief Construct a new Generic Lock object without acquiring a lock on the
   * mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   */
  GenericLock(GenericMutex& generic_mutex, const record_id_t& record_id,
              const transaction_id_t& transaction_id, const lock_mode_t& mode,
              DeferLockTag)
      : record_id_(record_id),
        transaction_id_(transaction_id),
        mode_(mode),
        generic_mutex_ptr_(&generic_mutex),
        owns_(false),
        denied_(false) {}

  /**
   * @brief Construct a new Generic Lock object assuming the caller already
   * acquired a lock on the mutex.
   *
   * @param generic_mutex Reference to the generic mutex to manager.
   * @param record_id Constant reference to the record identifier.
   * @param transaction_id Constant reference to the transaction identifier.
   * @param mode Constant reference to the lock mode.
   */
  GenericLock(GenericMutex& generic_mutex, const record_id_t& record_id,
              const transaction_id_t& transaction_id, const lock_mode_t& mode,
              AdoptLockTag)
      : record_id_(record_id),
        transaction_id_(transaction_id),
        mode_(mode),
        generic_mutex_ptr_(&generic_mutex),
        owns_(true),
        denied_(false) {}

  // Lock not copyable
  GenericLock(const GenericLock& other) = delete;

  /**
   * @brief Move construct a new Generic lock object.
   *
   * @param other Rvalue reference to the other generic lock object.
   */
  GenericLock(GenericLock&& other)
      : record_id_(other.record_id_),
        transaction_id_(other.transaction_id_),
        mode_(other.mode_),
        generic_mutex_ptr_(other.generic_mutex_ptr_),
        owns_(other.owns_),
        denied_(other.denied_) {
    other.owns_ = false;
    other.denied_ = false;
    other.generic_mutex_ptr_ = nullptr;
  }

  /**
   * @brief Destroy the Generic Lock object. The underlying mutex is unlocked if
   * locked.
   *
   */
  ~GenericLock() {
    if (owns_) {
      generic_mutex_ptr_->Unlock(record_id_, transaction_id_);
    }
  }

  /**
   * @brief Lock the underlying generic mutex.
   *
   * @returns `true` if the lock is acquired else `false`.
   */
  bool Lock() {
    if (generic_mutex_ptr_ == nullptr) {
      throw std::system_error(EPERM, std::system_category(),
                              "GenericLock::Lock: references null mutex");
    }
    if (owns_) {
      throw std::system_error(EDEADLK, std::system_category(),
                              "GenericLock::Lock: already locked");
    }
    owns_ = generic_mutex_ptr_->Lock(record_id_, transaction_id_, mode_);
    denied_ = !owns_;
    return owns_;
  }

  /**
   * @brief Unlock the underlying generic mutex.
   *
   */
  void Unlock() {
    if (!owns_) {
      throw std::system_error(EPERM, std::system_category(),
                              "GenericLock::UnLock: not locked");
    }
    owns_ = false;  // NOTE: No need to set denied_ as it will be already set to
                    // `false` since owns_ is set to `true`.
    generic_mutex_ptr_->Unlock(record_id_, transaction_id_);
  }

  /**
   * @brief Releases ownership of the associated generic mutex without
   * unlocking. If a lock is held prior to this call, the caller is now
   * responsible to unlock the mutex.
   *
   * @returns Pointer to the associated generic mutex or a null pointer.
   */
  GenericMutex* Release() {
    GenericMutex* rvalue = generic_mutex_ptr_;
    generic_mutex_ptr_ = nullptr;
    owns_ = false;
    denied_ = false;
    return rvalue;
  }

  /**
   * @brief Check if the lock on the underlying mutex has been denied. Note that
   * the mutex is not considered owned if the lock on it is denied.
   *
   * @returns `true` if lock is denied else `false`.
   */
  bool IsDenied() const { return denied_; }

  /**
   * @brief Check if the underlying generic mutex is owned. A mutex is owned
   * when its locked, and disowned upon unlock. Note that the mutex might get
   * failed to be owned if the lock on it is denied.
   *
   * @returns `true` if the mutex is owned else `false`.
   */
  bool OwnsLock() const { return owns_; }

  /**
   * @brief Get pointer to the underlying generic mutex.
   *
   * @returns Pointer to the underlying generic mutex.
   */
  GenericMutex* Mutex() const { return generic_mutex_ptr_; }

  /**
   * @brief Get the identifier of the record associated with the lock.
   *
   * @returns Constant reference to the record identifier.
   */
  const record_id_t& RecordId() const { return record_id_; }

  /**
   * @brief Get the lock mode.
   *
   * @returns Constant reference to the lock mode.
   */
  const lock_mode_t& LockMode() const { return mode_; }

  /**
   * @brief Get the identifier of the transaction associated with the lock.
   *
   * @returns Constant reference to the transaction identifier.
   */
  const transaction_id_t& TransactionId() const { return transaction_id_; }

  /**
   * @brief Check if the lock is owned.
   *
   * @returns `true` if the lock is owned and acquired else `false`.
   */
  explicit operator bool() const { return owns_; }

  // Lock not copy assignable
  GenericLock& operator=(const GenericLock& other) = delete;

  /**
   * @brief Move assign the generic lock.
   *
   * @param other Rvalue reference to the other generic lock object.
   * @returns Reference to the generic lock object.
   */
  GenericLock& operator=(GenericLock&& other) {
    if (owns_) {
      generic_mutex_ptr_->Unlock(record_id_, transaction_id_);
    }
    record_id_ = other.record_id_;
    transaction_id_ = other.transaction_id_;
    mode_ = other.mode_;
    generic_mutex_ptr_ = other.generic_mutex_ptr_;
    owns_ = other.owns_;
    denied_ = other.denied_;
    other.generic_mutex_ptr_ = nullptr;
    other.owns_ = false;
    other.denied_ = false;
    return *this;
  }

 private:
  record_id_t record_id_;
  transaction_id_t transaction_id_;
  lock_mode_t mode_;
  GenericMutex* generic_mutex_ptr_;
  bool owns_;
  bool denied_;
};

}  // namespace gl

#endif /* GENERIC_LOCK__GENERIC_LOCK_HPP */
