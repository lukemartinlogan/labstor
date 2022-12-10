//
// Created by lukemartinlogan on 11/3/22.
//

#include "basic_test.h"

#include <mpi.h>
#include <labstor/memory/memory_manager.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::MemoryManager;

struct SimpleHeader {
  labstor::ipc::Pointer p_;
};

TEST_CASE("MemoryManager") {
  int rank;
  char nonce = 8;
  size_t page_size = KILOBYTES(4);
  std::string shm_url = "test_mem_backend";
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  allocator_id_t alloc_id(0, 0);

  LABSTOR_ERROR_HANDLE_START()
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  if (rank == 0) {
    std::cout << "Creating SHMEM (rank 0): " << shm_url << std::endl;
    mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                            shm_url);
    mem_mngr->CreateAllocator(AllocatorType::kPageAllocator,
                              shm_url, alloc_id,
                              0, MemoryManager::kDefaultSlotSize);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Attaching SHMEM (rank 1): " << shm_url << std::endl;
    mem_mngr->AttachBackend(MemoryBackendType::kPosixShmMmap, shm_url);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Allocating pages (rank 0)" << std::endl;
    auto alloc = mem_mngr->GetAllocator(alloc_id);
    auto page = alloc->AllocatePtr<char>(page_size);
    memset(page, nonce, page_size);
    auto header = alloc->GetCustomHeader<SimpleHeader>();
    auto p1 = mem_mngr->Convert<void>(alloc_id, page);
    auto p2 = mem_mngr->Convert<void>(page);
    header->p_ = p1;
    REQUIRE(p1 == p2);
    REQUIRE(VerifyBuffer(page, page_size, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Finding and checking pages (rank 1)" << std::endl;
    auto alloc = mem_mngr->GetAllocator(alloc_id);
    auto header = alloc->GetCustomHeader<SimpleHeader>();
    auto page = alloc->Convert<char>(header->p_);
    REQUIRE(VerifyBuffer(page, page_size, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    mem_mngr->DestroyBackend(shm_url);
  }

  LABSTOR_ERROR_HANDLE_END()
}