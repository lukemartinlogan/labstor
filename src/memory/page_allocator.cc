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


#include <labstor/memory/allocator/page_allocator.h>

namespace labstor::ipc {

void* PageAllocator::GetFreeListStart() {
  return reinterpret_cast<void*>(
    custom_header_ + header_->custom_header_size_);;
}

void PageAllocator::shm_init(MemoryBackend *backend,
                             allocator_id_t id,
                             size_t custom_header_size,
                             size_t page_size,
                             size_t thread_table_size,
                             int concurrency,
                             size_t min_free_count) {
  backend_ = backend;
  header_ = reinterpret_cast<PageAllocatorHeader*>(backend_->data_);
  header_->Configure(id, custom_header_size, page_size,
                     thread_table_size, concurrency, min_free_count);
  custom_header_ = GetCustomHeader<char>();
  void *free_list_start = GetFreeListStart();
  // Create the array of free lists (which are queues)
  free_lists_.shm_init(free_list_start, header_->thread_table_size_);
  free_lists_.resize(concurrency);
  // Initialize each free list with an equal segment of the backend
  size_t cur_off = free_lists_.After() - backend_->data_;
  size_t cur_size = backend_->data_size_ - cur_off;
  size_t per_conc_region = cur_size / header_->concurrency_;
  if (per_conc_region < header_->page_size_) {
    throw NOT_ENOUGH_CONCURRENT_SPACE.format(
      "PageAllocator", backend_->data_size_, header_->concurrency_);
  }
  for (auto &free_list : free_lists_) {
    _queue<Page> q;
    q.shm_init(&free_list.queue_, backend_->data_);
    Allocator::ConstructObj<PageFreeList>(free_list);
    free_list.region_off_ = cur_off;
    free_list.region_size_ = per_conc_region;
    free_list.free_size_ = per_conc_region;
    free_list.total_alloced_ = 0;
    free_list.total_freed_ = 0;
    cur_off += per_conc_region;
  }
}

void PageAllocator::shm_deserialize(MemoryBackend *backend) {
  backend_ = backend;
  header_ = reinterpret_cast<PageAllocatorHeader*>(backend_->data_);
  custom_header_ = GetCustomHeader<char>();
  void *free_list_start = GetFreeListStart();
  free_lists_.shm_deserialize(free_list_start);
}

size_t PageAllocator::GetCurrentlyAllocatedSize() {
  size_t total_alloced = 0;
  size_t total_freed = 0;
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto &free_list = free_lists_[i];
    total_alloced += free_lists_[i].total_alloced_;
    total_freed += free_lists_[i].total_freed_;
  }
  return total_alloced - total_freed;
}

Pointer PageAllocator::Allocate(size_t size) {
  if (size > header_->page_size_) {
    throw PAGE_SIZE_UNSUPPORTED.format(size);
  }

  // Get current thread ID
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();

  // Allocate without lock if possible
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto free_list_id = (tid + i) % header_->concurrency_;
    auto &free_list = free_lists_[free_list_id];
    ScopedMutex list_lock(free_list.lock_);
    if (free_list.free_size_ < header_->page_size_) continue;
    if (list_lock.TryLock()) {
      Pointer p = _Allocate(free_list);
      if (!p.IsNull()) return p;
      _Borrow(&free_list, tid, false);
      p = _Allocate(free_list);
      return p;
    }
  }

  // Create a new allocator list, borrowing from an allocator with space
  _Borrow(nullptr, tid, true);

  return kNullPointer;
}

Pointer PageAllocator::AlignedAllocate(size_t size, size_t alignment) {
  throw ALIGNED_ALLOC_NOT_SUPPORTED.format();
}

bool PageAllocator::ReallocateNoNullCheck(Pointer &ptr, size_t new_size) {
  if (new_size > header_->page_size_) {
    throw PAGE_SIZE_UNSUPPORTED.format(new_size);
  }
  return false;
}

void PageAllocator::FreeNoNullCheck(Pointer &ptr) {
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto free_list_id = (tid + i) % header_->concurrency_;
    auto &free_list = free_lists_[free_list_id];
    ScopedMutex list_lock(free_list.lock_);
    if (list_lock.TryLock()) {
      _Free(&free_list, ptr);
      return;
    }
  }

  auto free_list_id = tid % header_->concurrency_;
  auto &free_list = free_lists_[free_list_id];
  ScopedMutex list_lock(free_list.lock_);
  list_lock.Lock();
  _Free(&free_list, ptr);
}

Pointer PageAllocator::_Allocate(PageFreeList &free_list) {
  // Dequeue a page from the free list
  _queue<Page> q;
  q.shm_deserialize(&free_list.queue_, backend_->data_);
  if (q.size()) {
    size_t off = q.dequeue_off();
    free_list.free_size_ -= header_->page_size_;
    free_list.total_alloced_ += header_->page_size_;
    return Pointer(GetId(), off);
  }

  // Create a new page from the segment
  if (free_list.region_size_ >= header_->page_size_) {
    size_t off = free_list.region_off_;
    free_list.region_size_ -= header_->page_size_;
    free_list.region_off_ += header_->page_size_;
    free_list.free_size_ -= header_->page_size_;
    free_list.total_alloced_ += header_->page_size_;
    return Pointer(GetId(), off);
  }

  return kNullPointer;
}

void PageAllocator::_Borrow(PageFreeList *to, tid_t tid, bool append) {
  tid += 1;
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto free_list_id = (tid + i) % header_->concurrency_;
    auto &from = free_lists_[free_list_id];
    ScopedMutex list_lock(from.lock_);
    if (from.free_size_ < header_->min_free_size_) continue;
    list_lock.Lock();
    if (from.free_size_ < header_->min_free_size_) continue;
    size_t borrow = from.free_size_ / 2 / header_->page_size_;
    if (append) to = &free_lists_[header_->concurrency_];
    for (auto j = 0; j < borrow; ++j) {
      Pointer p = _Allocate(from);
      _Free(to, p);
    }
  }
}

void PageAllocator::_Free(PageFreeList *free_list, Pointer &p) {
  _queue<Page> q;
  q.shm_deserialize(&free_list->queue_, backend_->data_);
  q.enqueue_off(p.off_);
  free_list->free_size_ += header_->page_size_;
  free_list->total_freed_ += header_->page_size_;
}

}  // namespace labstor::ipc
