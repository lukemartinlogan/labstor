/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
#define LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_

#include "allocator.h"
#include "labstor/thread/lock.h"
#include "labstor/data_structures/thread_unsafe/_queue.h"

namespace labstor::ipc {

typedef _queue_entry Page;

struct PageFreeList {
  Mutex lock_;                       /**< Always locks the list */
  _queue_header<Page> queue_;        /**< Singly linked list header */
  size_t region_off_, region_size_;  /**< Stack allocator */
  size_t free_size_;      /**< The number of bytes free in this list */
  size_t total_alloced_;  /**< Total number of bytes alloc'd from this list */
  size_t total_freed_;    /**< Total number of bytes freed to this list */
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
  PageAllocator()
  : header_(nullptr), custom_header_(nullptr) {}

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
  void shm_init(MemoryBackend *backend,
                allocator_id_t id,
                size_t custom_header_size = 0,
                size_t page_size = KILOBYTES(4),
                size_t thread_table_size = KILOBYTES(4),
                int concurrency = 8,
                size_t min_free_count = 16);

  /**
   * Attach an existing allocator from shared memory
   * */
  void shm_deserialize(MemoryBackend *backend) override;

  /**
   * Allocate a memory of \a size size. The page allocator cannot allocate
   * memory larger than the page size.
   * */
  Pointer Allocate(size_t size) override;

  /**
   * Allocate a memory of \a size size, which is aligned to \a
   * alignment.
   * */
  Pointer AlignedAllocate(size_t size, size_t alignment) override;

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

}  // namespace labstor::ipc

#endif  // LABSTOR_MEMORY_ALLOCATOR_PAGE_ALLOCATOR_H_
