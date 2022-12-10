//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_

#include "allocator.h"
#include "page_allocator.h"

namespace labstor::ipc {

class AllocatorFactory {
 public:

  /**
   * Create a new memory allocator
   * */
  template<typename ...Args>
  static std::unique_ptr<Allocator> Create(AllocatorType type,
                                           slot_id_t slot_id,
                                           MemoryBackend *backend,
                                           allocator_id_t alloc_id,
                                           size_t custom_header_size,
                                           Args&& ...args) {
    switch (type) {
      case AllocatorType::kPageAllocator: {
        auto alloc = std::make_unique<PageAllocator>(slot_id, backend);
        alloc->Create(alloc_id,
                      custom_header_size,
                      std::forward<Args>(args)...);
        return alloc;
      }
      default: return nullptr;
    }
  }

  /**
   * Attach to the existing allocator at \a slot slot on \a backend
   * memory backend
   * */
  static std::unique_ptr<Allocator> Attach(AllocatorType type,
                                           slot_id_t slot_id,
                                           MemoryBackend *backend) {
    switch (type) {
      case AllocatorType::kPageAllocator: {
        auto alloc = std::make_unique<PageAllocator>(slot_id, backend);
        alloc->Attach();
        return alloc;
      }
      default: return nullptr;
    }
  }
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
