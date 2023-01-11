//
// Created by lukemartinlogan on 1/10/23.
//

#include "test_init.h"
#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/memory/allocator/stack_allocator.h"

std::unique_ptr<void_allocator> alloc_inst_g;
std::unique_ptr<bipc::managed_shared_memory> segment_g;

void MainPretest() {
  // Boost shared memory
  bipc::shared_memory_object::remove("LabStorBoostBench");
  segment_g = std::make_unique<bipc::managed_shared_memory>(
    bipc::create_only, "LabStorBoostBench", GIGABYTES(4));
  alloc_inst_g = std::make_unique<void_allocator>(
    segment_g->get_segment_manager());

  // LabStor shared memory
  std::string shm_url = "LabStorSelfBench";
  allocator_id_t alloc_id(0, 1);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          MemoryManager::kDefaultBackendSize,
                          shm_url);
  mem_mngr->CreateAllocator<lipc::StackAllocator>(
    shm_url, alloc_id, 0);
}

void MainPosttest() {
  bipc::shared_memory_object::remove("LabstorBoostBench");
}