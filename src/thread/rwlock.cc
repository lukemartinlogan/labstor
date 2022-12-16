//
// Created by lukemartinlogan on 11/6/22.
//

#include "labstor/thread/lock/rwlock.h"

namespace labstor {

/**
 * Acquire the read lock
 * */
void RwLock::ReadLock() {
  bool ret;
  RwLockPayload expected, desired;
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  do {
    expected.as_int_ = payload_.load();
    if (expected.IsWriteLocked()) {
      thread_info->Yield();
      continue;
    }
    desired = expected;
    desired.bits.r_ += 1;
    ret = payload_.compare_exchange_weak(
      expected.as_int_,
      desired.as_int_);
  } while(!ret);
}

/**
 * Release the read lock
 * */
void RwLock::ReadUnlock() {
  bool ret;
  RwLockPayload expected, desired;
  do {
    expected.as_int_ = payload_.load();
    desired = expected;
    desired.bits.r_ -= 1;
    ret = payload_.compare_exchange_weak(
      expected.as_int_,
      desired.as_int_);
  } while(!ret);
}

/**
 * Acquire the write lock
 * */
void RwLock::WriteLock() {
  bool ret = false;
  RwLockPayload expected, desired;
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  do {
    expected.as_int_ = payload_.load();
    if (expected.IsReadLocked()) {
      thread_info->Yield();
      continue;
    }
    if (expected.IsWriteLocked()) {
      thread_info->Yield();
      continue;
    }
    desired = expected;
    desired.bits.w_ += 1;
    ret = payload_.compare_exchange_weak(
      expected.as_int_,
      desired.as_int_);
  } while(!ret);
}

/**
 * Release the write lock
 * */
void RwLock::WriteUnlock() {
  bool ret;
  RwLockPayload expected, desired;
  do {
    expected.as_int_ = payload_.load();
    desired = expected;
    desired.bits.w_ -= 1;
    ret = payload_.compare_exchange_weak(
      expected.as_int_,
      desired.as_int_);
  } while(!ret);
}

/**
 * SCOPED R/W READ LOCK
 * */

/**
 * Constructor
 * */
ScopedRwReadLock::ScopedRwReadLock(RwLock &lock)
: lock_(lock), is_locked_(false) {
}

/**
 * Release the read lock
 * */
ScopedRwReadLock::~ScopedRwReadLock() {
  Unlock();
}

/**
 * Acquire the read lock
 * */
void ScopedRwReadLock::Lock() {
  if (!is_locked_) {
    lock_.ReadLock();
    is_locked_ = true;
  }
}

/**
 * Release the read lock
 * */
void ScopedRwReadLock::Unlock() {
  if (is_locked_) {
    lock_.ReadUnlock();
    is_locked_ = false;
  }
}

/**
 * SCOPED R/W WRITE LOCK
 * */

/**
 * Constructor
 * */
ScopedRwWriteLock::ScopedRwWriteLock(RwLock &lock)
: lock_(lock), is_locked_(false) {
}

/**
 * Release the write lock
 * */
ScopedRwWriteLock::~ScopedRwWriteLock() {
  Unlock();
}

/**
 * Acquire the write lock
 * */
void ScopedRwWriteLock::Lock() {
  if (!is_locked_) {
    lock_.WriteLock();
    is_locked_ = true;
  }
}

/**
 * Release the write lock
 * */
void ScopedRwWriteLock::Unlock() {
  if (is_locked_) {
    lock_.WriteUnlock();
    is_locked_ = false;
  }
}

}  // namespace labstor