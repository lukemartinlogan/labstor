//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "omp.h"
#include <labstor/data_structures/lockless/vector.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

TEST_CASE("PageAllocator") {
  int count = 1024;
  std::string shm_url = "test_vector";
  allocator_id_t alloc_id(0, 0);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          shm_url);
  mem_mngr->CreateAllocator(AllocatorType::kPageAllocator,
                            shm_url,
                            alloc_id);
  auto alloc = mem_mngr->GetAllocator(alloc_id);

  // Allocate 1024 pages
  Pointer ps[count];
  void *ptrs[count], *min = nullptr, *max = nullptr;
  for (int i = 0; i < count; ++i) {
    if (i == 1023) {
      std::cout << "HERE" << std::endl;
    }
    ptrs[i] = alloc->AllocatePtr<void>(KILOBYTES(4), ps[i]);
    if (min == nullptr || ptrs[i] < min) {
      min = ptrs[i];
    }
    if (max == nullptr || ptrs[i] > max) {
      max = ptrs[i];
    }
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
    REQUIRE(ptrs[i] != nullptr);
  }

  // Verify these are at least 4KB apart
  for (int i = 1; i < 1024; ++i) {
    size_t p1 = reinterpret_cast<size_t>(ptrs[i]);
    size_t p2 = reinterpret_cast<size_t>(ptrs[i-1]);
    REQUIRE((p1 - p2) >= KILOBYTES(4));
  }

  //Convert process pointers into independent pointers
  for (int i = 0; i < count; ++i) {
    Pointer p = mem_mngr->Convert(ptrs[i]);
    REQUIRE(p == ps[i]);
  }

  // Free 1024 pages
  for (int i = 0; i < count; ++i) {
    if (i == 1023) {
      std::cout << "HERE" << std::endl;
    }
    alloc->Free(ps[i]);
  }

  // Reallocate 1024 pages
  for (int i = 0; i < count; ++i) {
    if (i == 1023) {
      std::cout << "HERE" << std::endl;
    }
    ptrs[i] = alloc->AllocatePtr<void>(KILOBYTES(4), ps[i]);
    if (i == 1023) {
      std::cout << i << std::endl;
      std::cout << ps[i].off_ << std::endl;
      std::cout << ptrs[i] << std::endl;
    }
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
    REQUIRE((min <= ptrs[i] && ptrs[i] <= max));
  }

  // Destory the SHM backend
  mem_mngr->DestroyBackend(shm_url);
}