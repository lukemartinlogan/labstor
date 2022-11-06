//
// Created by lukemartinlogan on 11/4/22.
//

#include <labstor/thread/mutex.h>

namespace labstor {

void Mutex::Lock() {
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  while (!TryLock()) {
    thread_info->Yield();
  }
}

bool Mutex::TryLock() {
  if (lock_.load() != 0) return false;
  uint32_t tkt = lock_.fetch_add(1);
  if (tkt != 0) {
    lock_.fetch_sub(1);
    return false;
  }
  return true;
}

void Mutex::Unlock() {
  lock_.fetch_sub(1);
}

ScopedMutex::ScopedMutex(Mutex &lock) : lock_(lock), is_locked_(false) {}

ScopedMutex::~ScopedMutex() {
  if (is_locked_) {
    lock_.Unlock();
  }
}

void ScopedMutex::Lock() {
  lock_.Lock();
}

bool ScopedMutex::TryLock() {
  is_locked_ = lock_.TryLock();
  return is_locked_;
}

}  // namespace labstor