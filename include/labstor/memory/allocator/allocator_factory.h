//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_

#include "allocator.h"
#include "fixed_fragmentation_allocator.h"
#include "page_allocator.h"

namespace labstor::memory {

class AllocatorFactory {
 public:
  static std::unique_ptr<Allocator> Get(AllocatorType type,
                                        slot_id_t slot_id,
                                        MemoryBackend *backend,
                                        size_t custom_header_size = 0) {
    switch (type) {
      case AllocatorType::kFixedFragmentationAllocator: {
        return std::make_unique<FixedFragmentationAllocator>(
          slot_id, backend, custom_header_size);
      }
      case AllocatorType::kPageAllocator: {
        return std::make_unique<PageAllocator>(
          slot_id, backend, custom_header_size);
      }
      default: return nullptr;
    }
  }
};

}  // namespace labstor::memory

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
