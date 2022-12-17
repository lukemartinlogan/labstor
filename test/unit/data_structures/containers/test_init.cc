//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "test_init.h"
#include <mpi.h>

#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/memory/allocator/page_allocator.h"

Allocator *alloc_g = nullptr;

void Pretest(AllocatorType type) {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 0);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          shm_url);
  mem_mngr->CreateAllocator(type, shm_url, alloc_id,
                            0, MemoryManager::kDefaultSlotSize);
  alloc_g = mem_mngr->GetAllocator(alloc_id);
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
  alloc_g = nullptr;
}

void MainPretest() {
  Pretest(AllocatorType::kPageAllocator);
}

void MainPosttest() {
  Posttest();
}