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


#include <mpi.h>
#include <iostream>
#include <labstor/memory/backend/posix_shm_mmap.h>
#include <assert.h>

using labstor::memory::PosixShmMmap;

bool VerifyBuffer(char *ptr, size_t size, char nonce) {
  for (size_t i = 0; i < size; ++i) {
    if (ptr[i] != nonce) {
      return false;
    }
  }
  return true;
}

int main(int argc, char **argv) {
  int rank;
  char nonce = 8;
  std::string shm_name = "test_mem_backend";
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  LABSTOR_ERROR_HANDLE_START()

  PosixShmMmap backend(shm_name);
  if (rank == 0) {
    std::cout << "Creating SHMEM (rank 0): " << shm_name << std::endl;
    backend.Create();
    backend.CreateSlot(MEGABYTES(1));
    auto &slot = backend.GetSlot(1);
    memset(slot.ptr_, nonce, slot.size_);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Attaching SHMEM (rank 1): " << shm_name << std::endl;
    backend.Attach();
    std::cout << "Attached header" << std::endl;
    auto &slot = backend.GetSlot(1);
    std::cout << "Acquired slot" << std::endl;
    char *ptr = reinterpret_cast<char*>(slot.ptr_);
    assert(VerifyBuffer(ptr, slot.size_, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Creating new slot (rank 0): " << shm_name << std::endl;
    backend.CreateSlot(MEGABYTES(1));
    auto &slot = backend.GetSlot(2);
    memset(slot.ptr_, nonce, slot.size_);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Getting new slot (rank 1): " << shm_name << std::endl;
    auto &slot = backend.GetSlot(2);
    char *ptr = reinterpret_cast<char*>(slot.ptr_);
    assert(VerifyBuffer(ptr, slot.size_, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Destroying shmem (rank 1): " << shm_name << std::endl;
    backend.Destroy();
  }
  MPI_Finalize();

  LABSTOR_ERROR_HANDLE_END()
}
