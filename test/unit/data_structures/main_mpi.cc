//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include "test_init.h"
#include <mpi.h>

#include <labstor/data_structures/lockless/vector.h>
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

Allocator *alloc_g = nullptr;

void Pretest(AllocatorType type) {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 0);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          shm_url);
  mem_mngr->CreateAllocator(type,
                            shm_url,
                            alloc_id);
  alloc_g = mem_mngr->GetAllocator(alloc_id);
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
  alloc_g = nullptr;
}

int main(int argc, char **argv) {
  int rc;
  MPI_Init(&argc, &argv);
  Catch::Session session;
  auto cli = session.cli();
  session.cli(cli);
  rc = session.applyCommandLine(argc, argv);
  if (rc != 0) return rc;
  Pretest(AllocatorType::kPageAllocator);
  int test_return_code = session.run();
  Posttest();
  if (rc != 0) return rc;
  MPI_Finalize();
  return test_return_code;
}