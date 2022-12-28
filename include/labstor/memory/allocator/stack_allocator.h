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


#ifndef LABSTOR_MEMORY_ALLOCATOR_STACK_ALLOCATOR_H_
#define LABSTOR_MEMORY_ALLOCATOR_STACK_ALLOCATOR_H_

#include "allocator.h"
#include "labstor/thread/lock.h"
#include "labstor/data_structures/thread_unsafe/_queue.h"

namespace labstor::ipc {

typedef _queue_entry Page;

struct StackAllocatorHeader : public AllocatorHeader {
  std::atomic<size_t> region_off_;
  std::atomic<size_t> region_size_;
  std::atomic<size_t> total_alloc_;

  StackAllocatorHeader() = default;

  void Configure(allocator_id_t alloc_id,
                 size_t custom_header_size,
                 size_t region_off,
                 size_t region_size) {
    AllocatorHeader::Configure(alloc_id, AllocatorType::kStackAllocator,
                               custom_header_size);
    region_off_ = region_off;
    region_size_ = region_size;
    total_alloc_ = 0;
  }
};

class StackAllocator : public Allocator {
 private:
  StackAllocatorHeader *header_;
  char *custom_header_;

 public:
  /**
   * Allocator constructor
   * */
  StackAllocator()
  : header_(nullptr), custom_header_(nullptr) {}

  /**
   * Determine the size of the shared-memory header
   * */
  size_t GetInternalHeaderSize() override {
    return sizeof(StackAllocatorHeader);
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
                size_t custom_header_size);

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
};

}  // namespace labstor::ipc

#endif  // LABSTOR_MEMORY_ALLOCATOR_STACK_ALLOCATOR_H_
