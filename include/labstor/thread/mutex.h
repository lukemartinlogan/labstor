//
// Created by lukemartinlogan on 11/3/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_

#include <atomic>
#include "thread_manager.h"

struct Mutex {
  std::atomic<uint32_t> lock_;

  Mutex() : lock_(0) {}

  void Lock() {
    auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
    while (!TryLock()) {
      thread_info->Yield();
    }
  }

  bool TryLock() {
    if (lock_.load() != 0) return false;
    uint32_t tkt = lock_.fetch_add(1);
    if (tkt != 0) {
      lock_.fetch_sub(1);
      return false;
    }
    return true;
  }

  void Unlock() {
    lock_.fetch_sub(1);
  }
};

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_MUTEX_H_
