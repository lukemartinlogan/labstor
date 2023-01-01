/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#include "basic_test.h"

#include <mpi.h>
#include "labstor/memory/memory_manager.h"

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
  allocator_id_t alloc_id(0, 1);

  LABSTOR_ERROR_HANDLE_START()
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  if (rank == 0) {
    std::cout << "Creating SHMEM (rank 0): " << shm_url << std::endl;
    mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                            MemoryManager::kDefaultBackendSize,
                            shm_url);
    mem_mngr->CreateAllocator(AllocatorType::kPageAllocator,
                              shm_url, alloc_id, 0);
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
