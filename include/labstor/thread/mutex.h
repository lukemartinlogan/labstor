//
// Created by lukemartinlogan on 11/3/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_

#include <atomic>
#include "thread_manager.h"

namespace labstor {

struct Mutex {
  std::atomic<uint32_t> lock_;

  Mutex() : lock_(0) {}

  void Lock();
  bool TryLock();
  void Unlock();
};

struct ScopedMutex {
  Mutex &lock_;
  bool is_locked_;

  explicit ScopedMutex(Mutex &lock);
  ~ScopedMutex();

  void Lock();
  bool TryLock();
};

union RwLockPayload {
  struct {
    uint32_t r_;
    uint32_t w_;
  } bits;
  uint64_t as_int_;

  inline bool IsWriteLocked() const {
    return bits.r_ == 0 && bits.w_ > 0;
  }

  inline bool IsReadLocked() const {
    return bits.r_ > 0 && bits.w_ == 0;
  }
};

struct RwLock {
  std::atomic<RwLockPayload> payload_;

  void ReadLock();
  void ReadUnlock();

  void WriteLock();
  void WriteUnlock();
};

struct ScopedRwLock {
  RwLock &lock_;
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
