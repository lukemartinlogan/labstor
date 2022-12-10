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

  PageAllocatorHeader() = default;

  void Configure(allocator_id_t alloc_id,
                 size_t custom_header_size,
                 size_t page_size,
                 size_t thread_table_size,
                 int concurrency,
                 size_t min_free_count) {
    AllocatorHeader::Configure(alloc_id, AllocatorType::kPageAllocator,
                               custom_header_size);
    page_size_ = page_size;
    thread_table_size_ = thread_table_size;
    concurrency_ = concurrency;
    min_free_count_ = min_free_count;
    min_free_size_ = page_size_ * min_free_count_;
  }
};

class PageAllocator : public Allocator {
 private:
  PageAllocatorHeader *header_;
  char *custom_header_;
  _array<PageFreeList> free_lists_;

 public:
  /**
   * Allocator constructor
   * */
  explicit PageAllocator(slot_id_t slot_id, MemoryBackend *backend) :
    header_(nullptr), custom_header_(nullptr), Allocator(slot_id, backend) {
  }

  /**
   * Determine the size of the shared-memory header
   * */
  size_t GetInternalHeaderSize() override {
    return sizeof(PageAllocatorHeader);
  }

  /**
   * Get the ID of this allocator from shared memory
   * */
  allocator_id_t GetId() override {
    return header_->allocator_id_;
  }

  /**
   * Initialize the allocator in shared memory
   * */
  void Create(allocator_id_t id,
              size_t custom_header_size = 0,
              size_t page_size = KILOBYTES(4),
              size_t thread_table_size = KILOBYTES(4),
              int concurrency = 8,
              size_t min_free_count = 16);

  /**
   * Attach an existing allocator from shared memory
   * */
  void Attach() override;

  /**
   * Allocate a memory of a \size size. The page allocator cannot allocate
   * memory larger than the page size.
   * */
  Pointer Allocate(size_t size) override;

  /**
   * Reallocate \a p pointer to \a new_size new size.
   *
   * @return whether or not the pointer p was changed
   * */
  bool ReallocateNoNullCheck(Pointer &p, size_t new_size) override;

  /**
   * Free \a ptr pointer. Null check is performed elsewhere.
   * */
  void FreeNoNullCheck(Pointer &ptr) override;

  /**
   * Get the current amount of data allocated. Can be used for leak
   * checking.
   * */
  size_t GetCurrentlyAllocatedSize() override;

 private:
  void* GetFreeListStart();
  Pointer _Allocate(PageFreeList &free_list);
  void _Borrow(PageFreeList *to, tid_t tid, bool append);
  void _Free(PageFreeList *free_list, Pointer &p);
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
