//
// Created by lukemartinlogan on 11/3/22.
//

#include <mpi.h>
#include <labstor/memory/memory_manager.h>

using labstor::memory::MemoryBackendType;
using labstor::memory::MemoryBackend;
using labstor::memory::allocator_id_t;
using labstor::memory::AllocatorType;
using labstor::memory::MemoryManager;

int main(int argc, char **argv) {
  int rank;
  char nonce = 8;
  std::string shm_url = "test_mem_backend";
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  LABSTOR_ERROR_HANDLE_START()
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  if (rank == 0) {
    std::cout << "Creating SHMEM (rank 0): " << shm_url << std::endl;
    mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                            shm_url);
    mem_mngr->CreateAllocator(AllocatorType::kPageAllocator,
                              shm_url,
                              allocator_id_t(0,0),
                              MemoryManager::kDefaultSlotSize);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Attaching SHMEM (rank 1): " << shm_url << std::endl;
    backend.Attach();
    std::cout << "Attached header" << std::endl;
    auto &slot = backend.GetSlot(1);
    std::cout << "Acquired slot" << std::endl;
    char *ptr = reinterpret_cast<char*>(slot.ptr_);
    assert(VerifyBuffer(ptr, slot.size_, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Creating new slot (rank 0): " << shm_url << std::endl;
    backend.CreateSlot(MEGABYTES(1));
    auto &slot = backend.GetSlot(2);
    memset(slot.ptr_, nonce, slot.size_);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank != 0) {
    std::cout << "Getting new slot (rank 1): " << shm_url << std::endl;
    auto &slot = backend.GetSlot(2);
    char *ptr = reinterpret_cast<char*>(slot.ptr_);
    assert(VerifyBuffer(ptr, slot.size_, nonce));
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Destroying shmem (rank 1): " << shm_url << std::endl;
    backend.Destroy();
  }
  MPI_Finalize();

  LABSTOR_ERROR_HANDLE_END()
}