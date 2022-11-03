//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_

#include "allocator.h"

namespace labstor::memory {

struct Page {
  size_t size_;
  size_t off_;
};

class PageAllocator : public Allocator {
 public:
  struct ShmSerialize {
    Pointer thread_lists_;
  };

 private:
  size_t growth_rate_, page_size_;
  ShmSerialize *header_;
  labstor::vector<labstor::list<Page>> thread_lists_;
  // in reality
  labstor::vector<labstor::list::ShmSerialize>

 public:
  PageAllocator(uint32_t id, MemoryBackend *backend) : Allocator(id, backend) {
  }

  void Create(size_t page_size, size_t growth_rate) {
  }

  void shm_serialize(PageAllocatorHeader *header) {
  }

  void shm_deserialize(PageAllocatorHeader *header) {
  }

  Pointer Allocate(size_t size) override {
  }

  void Free(Pointer &ptr) override {
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
