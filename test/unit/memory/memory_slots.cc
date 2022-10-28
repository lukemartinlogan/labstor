//
// Created by lukemartinlogan on 10/27/22.
//

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
    backend.Reserve(MEGABYTES(34));
    backend.MapSlot(MEGABYTES(1), true);
    auto &slot = backend.GetSlot(1);
    memset(slot.ptr_, nonce, slot.size_);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Attaching SHMEM (rank 1): " << shm_name << std::endl;
    backend.Attach();
    auto &slot = backend.GetSlot(1);
    char *ptr = reinterpret_cast<char*>(slot.ptr_);
    assert(VerifyBuffer(ptr, slot.size_, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Creating new slot (rank 0): " << shm_name << std::endl;
    backend.MapSlot(MEGABYTES(1), true);
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
