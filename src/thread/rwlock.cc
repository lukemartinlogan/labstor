//
// Created by lukemartinlogan on 11/6/22.
//

#include <labstor/thread/mutex.h>

namespace labstor {

void RwLock::ReadLock() {
  bool ret;
  RwLockPayload expected, desired;
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  do {
    expected = payload_.load();
    if (expected.IsWriteLocked()) {
      thread_info->Yield();
      continue;
    }
    desired = expected;
    desired.bits.r_ += 1;
    ret = payload_.compare_exchange_weak(expected, desired);
  } while(!ret);
}

void RwLock::ReadUnlock() {
  bool ret;
  RwLockPayload expected, desired;
  do {
    expected = payload_.load();
    desired = expected;
    desired.bits.r_ -= 1;
    ret = payload_.compare_exchange_weak(expected, desired);
  } while(!ret);
}

void RwLock::WriteLock() {
  bool ret;
  RwLockPayload expected, desired;
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  do {
    expected = payload_.load();
    if (expected.IsReadLocked()) {
      thread_info->Yield();
      continue;
    }
    desired = expected;
    desired.bits.w_ += 1;
    ret = payload_.compare_exchange_weak(expected, desired);
  } while(!ret);
}

void RwLock::WriteUnlock() {
  bool ret;
  RwLockPayload expected, desired;
  do {
    expected = payload_.load();
    desired = expected;
    desired.bits.w_ -= 1;
    ret = payload_.compare_exchange_weak(expected, desired);
  } while(!ret);
}

}