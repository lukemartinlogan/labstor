//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_

#include "allocator.h"
#include <labstor/thread/mutex.h>

namespace labstor::memory {

struct Page {
  size_t next_;
};

struct PageFreeList {
  Mutex lock_;
  size_t head_, tail_, length_;
};

struct PageAllocatorHeader {
  size_t page_size_;
  size_t thread_table_size_;
};

class PageAllocator : public Allocator {
 private:
  size_t page_size_, thread_table_size_;
  PageAllocatorHeader *header_;
  Array<PageFreeList> free_lists_;

 public:
  PageAllocator(slot_id_t slot_id, MemoryBackend *backend,
                size_t custom_header_size,
                size_t page_size = KILOBYTES(4),
                size_t thread_table_size = KILOBYTES(4)) :
    Allocator(slot_id, backend, custom_header_size),
    page_size_(page_size), thread_table_size_(thread_table_size) {
  }

  void Create(allocator_id_t id) override {
    header_ = reinterpret_cast<PageAllocatorHeader*>(slot_.ptr_);
    header_->page_size_ = page_size_;
    header_->thread_table_size_ = thread_table_size_;
    free_lists_.Create(reinterpret_cast<void*>(header_ + 1),
                       thread_table_size_);
    for (auto &free_list : free_lists_) {
      memset(&free_list, 0, sizeof(free_list));
    }
  }

  void Attach() {
    header_ = reinterpret_cast<PageAllocatorHeader*>(slot_.ptr_);
    free_lists_.Attach(reinterpret_cast<void*>(header_ + 1));
  }

  Pointer Allocate(size_t size) override {

  }

  void Free(Pointer &ptr) override {
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
