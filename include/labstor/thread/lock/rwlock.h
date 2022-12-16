//
// Created by lukemartinlogan on 12/13/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_RWLOCK_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_RWLOCK_H_

#include <atomic>
#include "labstor/thread/thread_manager.h"

namespace labstor {

union RwLockPayload {
  struct {
    uint32_t r_;
    uint32_t w_;
  } bits_;
  uint64_t as_int_;

  RwLockPayload() = default;

  RwLockPayload(const RwLockPayload &other) {
    as_int_ = other.as_int_;
  }

  RwLockPayload(uint64_t other) {
    as_int_ = other;
  }

  bool IsWriteLocked() const {
    return bits_.w_ > 0;
  }

  bool IsReadLocked() const {
    return bits_.r_ > 0;
  }
};

struct RwLock {
  std::atomic<uint64_t> payload_;

  RwLock() : payload_(0) {}

  void ReadLock();
  void ReadUnlock();

  void WriteLock();
  void WriteUnlock();

  void assert_r_refcnt(int ref);
  void assert_w_refcnt(int ref);
};

struct ScopedRwReadLock {
  RwLock &lock_;
  bool is_locked_;

  explicit ScopedRwReadLock(RwLock &lock);
  ~ScopedRwReadLock();

  void Lock();
  void Unlock();
};

struct ScopedRwWriteLock {
  RwLock &lock_;
  bool is_locked_;

  explicit ScopedRwWriteLock(RwLock &lock);
  ~ScopedRwWriteLock();

  void Lock();
  void Unlock();
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_RWLOCK_H_
