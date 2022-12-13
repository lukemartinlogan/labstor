//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "omp.h"
#include "labstor/thread/lock.h"

using labstor::Mutex;
using labstor::RwLock;

void MutexTest() {
  int nthreads = 4;
  int count = 0;
  Mutex lock;

  omp_set_dynamic(0);
#pragma omp parallel shared(lock) num_threads(nthreads)
  {
    // Support parallel write
#pragma omp barrier
    for(int i = 0; i < 10000; ++i) {
      lock.Lock();
      count += 1;
      lock.Unlock();
    }
#pragma omp barrier
    REQUIRE(count == 40000);
#pragma omp barrier
  }
}

void RwLockTest() {
  int nthreads = 4;
  int count = 0;
  RwLock lock;

  omp_set_dynamic(0);
#pragma omp parallel shared(lock) num_threads(nthreads)
  {
    // Support parallel write
#pragma omp barrier
    for(int i = 0; i < 10000; ++i) {
      lock.WriteLock();
      count += 1;
      lock.WriteUnlock();
    }
#pragma omp barrier
    REQUIRE(count == 40000);
#pragma omp barrier
  }
}

TEST_CASE("Mutex") {
  MutexTest();
}

TEST_CASE("RwLock") {
  RwLockTest();
}