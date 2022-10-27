//
// Created by lukemartinlogan on 10/26/22.
//

#include <labstor/memory/backend/shared_memory_mmap.h>

using labstor::memory::MemorySlot;
using labstor::memory::SharedMemoryMmap;

#define MEGABYTES(n) ((uint64_t)n*(1<<20))
#define GIGABYTES(n) ((uint64_t)n*(1<<30))

int main() {
  SharedMemoryMmap b1("shmem_test");
  // Reserve 8GB of SHMEM
  b1.Reserve(GIGABYTES(8));
  sleep(2);

  // Map 8GB of SHMEM
  b1.MapSlot(GIGABYTES(8));
  sleep(2);

  // Set 2GB of SHMEM
  auto &slot = b1.GetSlot(0);
  memset(slot.ptr_, 0, GIGABYTES(2));
  sleep(2);

  // Destroy SHMEM
  b1.Destroy();
}