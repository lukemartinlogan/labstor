//
// Created by lukemartinlogan on 12/16/22.
//

#include "test_init.h"

void MultiThreadedPageAllocationTest(Allocator *alloc) {
  int nthreads = 8;
  LABSTOR_THREAD_MANAGER->GetThreadStatic();

  omp_set_dynamic(0);
#pragma omp parallel shared(alloc) num_threads(nthreads)
  {
#pragma omp barrier
    PageAllocationTest(alloc);
#pragma omp barrier
  }
}

TEST_CASE("PageAllocatorMultithreaded") {
  auto alloc = Pretest(AllocatorType::kPageAllocator);
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  MultiThreadedPageAllocationTest(alloc);
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  Posttest();
}