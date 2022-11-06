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
#include <iostream>
#include <labstor/memory/backend/posix_shm_mmap.h>

using labstor::memory::PosixShmMmap;

TEST_CASE("MemorySlot") {
  int rank;
  char nonce = 8;
  std::string shm_url = "test_mem_backend";
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  LABSTOR_ERROR_HANDLE_START()

  PosixShmMmap backend(shm_url);
  if (rank == 0) {
    std::cout << "HERE?" << std::endl;
    SECTION("Creating SHMEM (rank 0)") {
      backend.Create();
      backend.CreateSlot(MEGABYTES(1));
      auto &slot = backend.GetSlot(1);
      memset(slot.ptr_, nonce, slot.size_);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    SECTION("Attaching SHMEM (rank 1)") {
      backend.Attach();
      auto &slot = backend.GetSlot(1);
      char *ptr = reinterpret_cast<char *>(slot.ptr_);
      REQUIRE(VerifyBuffer(ptr, slot.size_, nonce));
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    SECTION("Creating new slot (rank 0)") {
      backend.CreateSlot(MEGABYTES(1));
      auto &slot = backend.GetSlot(2);
      memset(slot.ptr_, nonce, slot.size_);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    SECTION("Getting new slot (rank 1)") {
      auto &slot = backend.GetSlot(2);
      char *ptr = reinterpret_cast<char *>(slot.ptr_);
      REQUIRE(VerifyBuffer(ptr, slot.size_, nonce));
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    SECTION("Destroying shmem (rank 1)") {
      backend.Destroy();
    }
  }

  LABSTOR_ERROR_HANDLE_END()
}
