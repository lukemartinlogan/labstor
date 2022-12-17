//
// Created by lukemartinlogan on 12/16/22.
//

#include "test_init.h"

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

void MainPretest() {}

void MainPosttest() {}