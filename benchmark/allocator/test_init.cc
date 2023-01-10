//
// Created by lukemartinlogan on 1/10/23.
//

#include "test_init.h"
#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/memory/allocator/stack_allocator.h"

std::string shm_url = "LabStorSelfBench";

void MainPretest() {
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          MemoryManager::kDefaultBackendSize,
                          shm_url);
}

void MainPosttest() {
}