//
// Created by lukemartinlogan on 11/3/22.
//

#include "basic_test.h"

#include <mpi.h>
#include <labstor/memory/memory_manager.h>

using labstor::memory::MemoryBackendType;
using labstor::memory::MemoryBackend;
using labstor::memory::allocator_id_t;
using labstor::memory::AllocatorType;
using labstor::memory::MemoryManager;

TEST_CASE("MemoryManager") {
  int rank;
  char nonce = 8;
  std::string shm_url = "test_mem_backend";\
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  allocator_id_t alloc_id(0, 0);

  LABSTOR_ERROR_HANDLE_START()
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  if (rank == 0) {
    std::cout << "Creating SHMEM (rank 0): " << shm_url << std::endl;
    mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                            shm_url);
    mem_mngr->CreateAllocator(AllocatorType::kPageAllocator,
                              shm_url,
                              alloc_id);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Attaching SHMEM (rank 1): " << shm_url << std::endl;
    mem_mngr->AttachBackend(MemoryBackendType::kPosixShmMmap, shm_url);
    auto alloc = mem_mngr->GetAllocator(alloc_id);

  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Allocating pages (rank 0)" << std::endl;
    auto alloc = mem_mngr->GetAllocator(alloc_id);
    auto page = alloc->Allocate(KILOBYTES(4));
  }

  LABSTOR_ERROR_HANDLE_END()
}