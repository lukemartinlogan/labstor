//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "omp.h"
#include "labstor/thread/lock.h"

using labstor::Mutex;
using labstor::RwLock;

void MutexTest() {
  int nthreads = 8;
  int loop_count = 10000;
  int count = 0;
  Mutex lock;

  LABSTOR_THREAD_MANAGER->GetThreadStatic();

  omp_set_dynamic(0);
#pragma omp parallel shared(lock) num_threads(nthreads)
  {
    // Support parallel write
#pragma omp barrier
    for(int i = 0; i < loop_count; ++i) {
      lock.Lock();
      count += 1;
      lock.Unlock();
    }
#pragma omp barrier
    REQUIRE(count == loop_count * nthreads);
#pragma omp barrier
  }
}

void barrier_for_reads(int *tid_start, int left) {
  int count;
  do {
    count = 0;
    for (int i = 0; i < left; ++i) {
      count += tid_start[i];
    }
  } while(count < left);
}

void RwLockTest() {
  int nthreads = 8;
  int left = nthreads / 2;
  int tid_start[left] = {0};
  int loop_count = 100000;
  int count = 0;
  RwLock lock;

  LABSTOR_THREAD_MANAGER->GetThreadStatic();

  omp_set_dynamic(0);
#pragma omp parallel shared(lock,nthreads,left,loop_count,count,tid_start) num_threads(nthreads)
  {
    int tid = omp_get_thread_num();

    // Support parallel write
#pragma omp barrier
    for(int i = 0; i < loop_count; ++i) {
      lock.WriteLock();
      count += 1;
      lock.WriteUnlock();
    }
#pragma omp barrier
    REQUIRE(count == loop_count * nthreads);
#pragma omp barrier

    // Support for parallel read and write
#pragma omp barrier

    int cur_count = count;
    if (tid < left) {
      lock.ReadLock();
      tid_start[tid] = 1;
      barrier_for_reads(tid_start, left);
      for (int i = 0; i < loop_count; ++i) {
        REQUIRE(count == cur_count);
      }
      lock.ReadUnlock();
    } else {
      barrier_for_reads(tid_start, left);
      lock.WriteLock();
      for (int i = 0; i < loop_count; ++i) {
        count += 1;
      }
      lock.WriteUnlock();
    }
  }
}

TEST_CASE("Mutex") {
  MutexTest();
}

TEST_CASE("RwLock") {
  RwLockTest();
}