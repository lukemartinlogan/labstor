//
// Created by lukemartinlogan on 10/29/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_FIXED_FRAGMENTATION_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_FIXED_FRAGMENTATION_ALLOCATOR_H_

#include <labstor/types/basic.h>
#include "allocator.h"
// #include "page_allocator.h"
#include <labstor/data_structures/unordered_map.h>

namespace labstor::ipc {

/**
 * unordered_map<offset, MemoryRegion>
 * unordered_map<size, list<MemoryRegion>>
 * */

class FixedFragmentationAllocator : public Allocator {
 private:
  size_t base_;
  float growth_;
  int alignment_;

 public:
  FixedFragmentationAllocator(slot_id_t slot_id, MemoryBackend *backend,
                              size_t custom_header_size = 0) :
    base_(64), growth_(1.2), alignment_(8),
    Allocator(slot_id, backend) {}

  size_t GetInternalHeaderSize() override {
    return 0;
  }

  allocator_id_t GetId() override {
    return allocator_id_t();
  }

  void Create(allocator_id_t id) override {
  }

  void Attach() override {
  }

  size_t GetCurrentlyAllocatedSize() override {
    return 0;
  }

  Pointer Allocate(size_t size) override {
    return Pointer();
  }

  void Free(Pointer &ptr) override {
  }
  
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_FIXED_FRAGMENTATION_ALLOCATOR_H_
