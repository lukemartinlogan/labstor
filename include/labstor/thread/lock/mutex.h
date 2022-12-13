//
// Created by lukemartinlogan on 11/3/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_

#include <atomic>
#include "labstor/thread/thread_manager.h"

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
  void Unlock();
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
