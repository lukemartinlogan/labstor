//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_

#include "allocator/allocator.h"
#include "backend/memory_backend.h"

namespace labstor::memory {

class MemoryManager {
 private:
  std::unordered_map<std::string, std::unique_ptr<MemoryBackend>> backends_;
  std::unordered_map<allocator_id_t, std::unique_ptr<Allocator>> allocators_;

 public:
  static const size_t kDefaultSlotSize = GIGABYTES(64);

  MemoryBackend* CreateBackend(MemoryBackendType type, const std::string &url);
  MemoryBackend* AttachBackend(MemoryBackendType type, const std::string &url);
  MemoryBackend* GetBackend(const std::string &url);
  void ScanBackends();
  Allocator* CreateAllocator(AllocatorType type,
                             const std::string &url,
                             allocator_id_t alloc_id,
                             size_t slot_size = kDefaultSlotSize,
                             size_t custom_header_size = 0);
  Allocator* GetAllocator(allocator_id_t alloc_id);

  template<typename T>
  T* Convert(Pointer &p) {
    return allocators_[p.allocator_id_]->Convert<T>(p);
  }
};

}  // namespace labstor::memory

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_
