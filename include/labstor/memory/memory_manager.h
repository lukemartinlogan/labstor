//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_

#include "allocator/allocator.h"
#include "backend/memory_backend.h"

namespace labstor::ipc {

class MemoryManager {
 private:
  std::unordered_map<std::string, std::unique_ptr<MemoryBackend>> backends_;
  std::unordered_map<allocator_id_t, std::unique_ptr<Allocator>> allocators_;

 public:
  static const size_t kDefaultSlotSize = GIGABYTES(64);

  MemoryBackend* CreateBackend(MemoryBackendType type, const std::string &url);
  MemoryBackend* AttachBackend(MemoryBackendType type, const std::string &url);
  MemoryBackend* GetBackend(const std::string &url);
  void DestroyBackend(const std::string &url);
  void ScanBackends();
  Allocator* CreateAllocator(AllocatorType type,
                             const std::string &url,
                             allocator_id_t alloc_id,
                             size_t slot_size = kDefaultSlotSize,
                             size_t custom_header_size = 0);
  Allocator* DefineAllocator(AllocatorType type,
                             const std::string &url,
                             allocator_id_t alloc_id,
                             size_t slot_size = kDefaultSlotSize,
                             size_t custom_header_size = 0);
  Allocator* GetAllocator(allocator_id_t alloc_id);

  template<typename T, typename ...Args>
  Allocator* ConfigureAllocator(allocator_id_t allocator_id, Args ...args) {
    T *allocator = dynamic_cast<T*>(GetAllocator(allocator_id));
    allocator->Configure(args...);
    return allocator;
  }

  template<typename T>
  T* Convert(Pointer &p) {
    if (p == kNullPointer) {
      return nullptr;
    }
    return GetAllocator(p.allocator_id_)->Convert<T>(p);
  }

  template<typename T>
  Pointer Convert(allocator_id_t allocator_id, T *ptr) {
    return GetAllocator(allocator_id)->Convert(ptr);
  }

  template<typename T>
  Pointer Convert(T *ptr) {
    for (auto &[alloc_id, alloc] : allocators_) {
      if (alloc->ContainsPtr(ptr)) {
        return alloc->Convert(ptr);
      }
    }
    return kNullPointer;
  }
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_MANAGER_H_
