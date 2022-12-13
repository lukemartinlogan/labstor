//
// Created by lukemartinlogan on 11/4/22.
//

#include "labstor/thread/lock.h"

namespace labstor {

/**
 * Acquire the mutex
 * */
void Mutex::Lock() {
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  while (!TryLock()) {
    thread_info->Yield();
  }
}

/**
 * Attempt to acquire the mutex
 * */
bool Mutex::TryLock() {
  if (lock_.load() != 0) return false;
  uint32_t tkt = lock_.fetch_add(1);
  if (tkt != 0) {
    lock_.fetch_sub(1);
    return false;
  }
  return true;
}

/**
 * Release the mutex
 * */
void Mutex::Unlock() {
  lock_.fetch_sub(1);
}

/**
 * SCOPED MUTEX
 * */

/**
 * Constructor
 * */
ScopedMutex::ScopedMutex(Mutex &lock)
: lock_(lock), is_locked_(false) {
}

/**
 * Release the mutex
 * */
ScopedMutex::~ScopedMutex() {
  Unlock();
}

/**
 * Acquire the mutex
 * */
void ScopedMutex::Lock() {
  if (!is_locked_) {
    lock_.Lock();
    is_locked_ = true;
  }
}

/**
 * Attempt to acquire the mutex
 * */
bool ScopedMutex::TryLock() {
  if (!is_locked_) {
    is_locked_ = lock_.TryLock();
  }
  return is_locked_;
}

/**
 * Acquire the mutex
 * */
void ScopedMutex::Unlock() {
  if (is_locked_) {
    lock_.Unlock();
    is_locked_ = false;
  }
}

}  // namespace labstor