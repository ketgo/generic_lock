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

/**
 * @brief Uint Test Generic Lock
 *
 */

#include <gtest/gtest.h>

#include <generic_lock/generic_lock.hpp>

using namespace gl;

class GenericLockTestFixture : public ::testing::Test {
 protected:
  typedef size_t RecordId;
  typedef size_t ThreadId;
  enum class LockMode { READ, WRITE };

  const RecordId record_id = 1;
  const LockMode mode = LockMode::WRITE;
  const ThreadId thread_id = 1;
  class MockMutex {
   public:
    typedef RecordId record_id_t;
    typedef LockMode lock_mode_t;
    typedef ThreadId thread_id_t;

    MockMutex() : _locked(false) {}
    bool Lock(const record_id_t& record_id, const lock_mode_t& mode,
              const thread_id_t& thread_id) {
      if (_locked) {
        throw std::system_error(EDEADLK, std::system_category(),
                                "MockMutex::Lock: already locked");
      }
      if (record_id == 1) {  // Locks only record identifier `1`.
        _locked = true;
      }
      return _locked;
    }
    void Unlock(const record_id_t& record_id, const thread_id_t& thread_id) {
      if (!_locked) {
        throw std::system_error(EPERM, std::system_category(),
                                "MockMutex::UnLock: not locked");
      }
      _locked = false;
    }
    bool IsLocked() const { return _locked; }

   private:
    bool _locked;
  } mutex;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(GenericLockTestFixture, TestDefaultConstructor) {
  GenericLock<MockMutex> lock;
  ASSERT_FALSE(lock.OwnsLock());
  ASSERT_FALSE(lock.IsDenied());
  ASSERT_FALSE(lock);
}

TEST_F(GenericLockTestFixture, TestConstructorWithOwning) {
  GenericLock<MockMutex> lock(mutex, record_id, mode, thread_id);
  ASSERT_TRUE(lock.OwnsLock());
  ASSERT_FALSE(lock.IsDenied());
  ASSERT_TRUE(lock);
  ASSERT_TRUE(mutex.IsLocked());
}

TEST_F(GenericLockTestFixture, TestConstructorFailedOwning) {
  GenericLock<MockMutex> lock(mutex, 2, mode, thread_id);
  ASSERT_FALSE(lock.OwnsLock());
  ASSERT_TRUE(lock.IsDenied());
  ASSERT_FALSE(lock);
  ASSERT_FALSE(mutex.IsLocked());
}

TEST_F(GenericLockTestFixture, TestConstructorDeferredOwning) {
  GenericLock<MockMutex> lock(mutex, record_id, mode, thread_id, DeferLock);
  ASSERT_FALSE(lock.OwnsLock());
  ASSERT_FALSE(lock.IsDenied());
  ASSERT_FALSE(lock);
  ASSERT_FALSE(mutex.IsLocked());
}

TEST_F(GenericLockTestFixture, TestConstructorAdoptOwning) {
  mutex.Lock(record_id, mode, thread_id);
  GenericLock<MockMutex> lock(mutex, record_id, mode, thread_id, AdoptLock);
  ASSERT_TRUE(lock.OwnsLock());
  ASSERT_FALSE(lock.IsDenied());
  ASSERT_TRUE(lock);
  ASSERT_TRUE(mutex.IsLocked());
}

TEST_F(GenericLockTestFixture, TestLock) {}