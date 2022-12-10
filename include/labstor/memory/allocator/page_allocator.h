//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_

#include "allocator.h"
#include <labstor/thread/mutex.h>
#include "labstor/data_structures/thread_unsafe/_queue.h"

namespace labstor::ipc {

typedef _queue_entry Page;

struct PageFreeList {
  Mutex lock_;
  _queue_header queue_;
  size_t region_off_, region_size_;
  size_t free_size_, alloc_size_;
};

struct PageAllocatorHeader : public AllocatorHeader {
  size_t page_size_, min_free_count_, min_free_size_;
  int concurrency_;
  size_t thread_table_size_;
  Mutex lock_;

  explicit PageAllocatorHeader(size_t custom_header_size) :
    page_size_(KILOBYTES(4)), thread_table_size_(KILOBYTES(4)),
    concurrency_(4), min_free_count_(16),
    min_free_size_(page_size_ * min_free_count_),
    AllocatorHeader(AllocatorType::kPageAllocator, custom_header_size) {}

  void Configure(size_t page_size, size_t thread_table_size,
                 int concurrency, size_t min_free_count) {
    page_size_ = page_size;
    thread_table_size_ = thread_table_size;
    concurrency_ = concurrency;
    min_free_count_ = min_free_count;
    min_free_size_ = page_size_ * min_free_count_;
  }
};

class PageAllocator : public Allocator {
 private:
  PageAllocatorHeader params_;
  PageAllocatorHeader *header_;
  char *custom_header_;
  _array<PageFreeList> free_lists_;

 public:
  explicit PageAllocator(slot_id_t slot_id, MemoryBackend *backend,
                         size_t custom_header_size) :
    params_(custom_header_size), header_(nullptr), custom_header_(nullptr),
    Allocator(slot_id, backend) {}

  size_t GetInternalHeaderSize() override {
    return sizeof(PageAllocatorHeader);
  }
  allocator_id_t GetId() override {
    return header_->allocator_id_;
  }

  void Configure(size_t page_size, size_t thread_table_size,
                 int concurrency, size_t min_free_count);
  void Create(allocator_id_t id) override;
  void Attach() override;
  Pointer Allocate(size_t size) override;
  bool ReallocateNoNullCheck(Pointer &p, size_t new_size) override;
  void FreeNoNullCheck(Pointer &ptr) override;
  size_t GetCurrentlyAllocatedSize() override;

 private:
  void* GetFreeListStart();
  Pointer _Allocate(PageFreeList &free_list);
  void _Borrow(PageFreeList *to, tid_t tid, bool append);
  void _Free(PageFreeList *free_list, Pointer &p);
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
