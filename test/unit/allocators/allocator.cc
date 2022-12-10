//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "omp.h"
#include "labstor/data_structures/thread_unsafe/vector.h"
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

Allocator* Pretest(AllocatorType type) {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 0);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          shm_url);
  mem_mngr->CreateAllocator(type, shm_url, alloc_id,
                            0, MemoryManager::kDefaultSlotSize);
  auto alloc = mem_mngr->GetAllocator(alloc_id);
  return alloc;
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
}

void PageAllocationTest(Allocator *alloc) {
  int count = 1024;
  size_t page_size = KILOBYTES(4);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);

  // Allocate pages
  Pointer ps[count];
  void *ptrs[count];
  for (int i = 0; i < count; ++i) {
    ptrs[i] = alloc->AllocatePtr<void>(page_size, ps[i]);
    memset(ptrs[i], i, page_size);
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
    REQUIRE(ptrs[i] != nullptr);
  }

  //Convert process pointers into independent pointers
  for (int i = 0; i < count; ++i) {
    Pointer p = mem_mngr->Convert(ptrs[i]);
    REQUIRE(p == ps[i]);
    REQUIRE(VerifyBuffer((char*)ptrs[i], page_size, i));
  }

  // Free pages
  for (int i = 0; i < count; ++i) {
    alloc->Free(ps[i]);
  }

  // Reallocate pages
  for (int i = 0; i < count; ++i) {
    ptrs[i] = alloc->AllocatePtr<void>(page_size, ps[i]);
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
  }

  // Free again
  for (int i = 0; i < count; ++i) {
    alloc->Free(ps[i]);
  }
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

void MultiThreadedPageAllocationTest(Allocator *alloc) {
  int nthreads = 4;

  omp_set_dynamic(0);
#pragma omp parallel shared(alloc) num_threads(nthreads)
  {
#pragma omp barrier
    PageAllocationTest(alloc);
  }
}

TEST_CASE("PageAllocator") {
  auto alloc = Pretest(AllocatorType::kPageAllocator);
  PageAllocationTest(alloc);
  Posttest();
}

TEST_CASE("PageAllocatorMultithreaded") {
  auto alloc = Pretest(AllocatorType::kPageAllocator);
  MultiThreadedPageAllocationTest(alloc);
  Posttest();
}