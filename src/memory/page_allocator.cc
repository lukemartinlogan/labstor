//
// Created by lukemartinlogan on 11/3/22.
//

#include <labstor/memory/allocator/page_allocator.h>

namespace labstor::ipc {

void PageAllocator::Configure(size_t page_size, size_t thread_table_size,
                              int concurrency, size_t min_free_count) {
  params_.Configure(page_size, thread_table_size, concurrency, min_free_count);
}

void* PageAllocator::GetFreeListStart() {
  return reinterpret_cast<void*>(
    custom_header_ + header_->custom_header_size_);;
}

void PageAllocator::Create(allocator_id_t id) {
  header_ = reinterpret_cast<PageAllocatorHeader*>(slot_.ptr_);
  params_.allocator_id_ = id;
  memcpy(header_, &params_, sizeof(params_));
  custom_header_ = GetCustomHeader<char>();
  void *free_list_start = GetFreeListStart();
  free_lists_.Create(free_list_start, header_->thread_table_size_);
  for (auto &free_list : free_lists_) {
    _queue<Page> q;
    q.Create(&free_list.queue_, slot_.ptr_);
  }
  size_t cur_off = free_lists_.After() - slot_.ptr_;
  size_t cur_size = slot_.size_ - cur_off;
  size_t per_conc_region = cur_size / header_->concurrency_;
  if (per_conc_region == 0) {
    throw NOT_ENOUGH_CONCURRENT_SPACE.format(
      "PageAllocator", slot_.size_, header_->concurrency_);
  }
  for (auto i = 0; i < header_->concurrency_; ++i) {
    free_lists_[i].region_off_ = cur_off;
    free_lists_[i].region_size_ = per_conc_region;
    free_lists_[i].free_size_ = per_conc_region;
    free_lists_[i].alloc_size_ = 0;
    cur_off += per_conc_region;
  }
}

void PageAllocator::Attach() {
  header_ = reinterpret_cast<PageAllocatorHeader*>(slot_.ptr_);
  custom_header_ = GetCustomHeader<char>();
  void *free_list_start = GetFreeListStart();
  free_lists_.Attach(free_list_start);
}

size_t PageAllocator::GetCurrentlyAllocatedSize() {
  size_t size = 0;
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto &free_list = free_lists_[i];
    size += free_list.alloc_size_;
  }
  return size;
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
    auto list_lock = ScopedMutex(free_list.lock_);
    if (free_list.free_size_ < header_->page_size_) continue;
    if (list_lock.TryLock()) {
      Pointer p = _Allocate(free_list);
      if (!p.is_null()) return p;
      _Borrow(&free_list, tid, false);
      p = _Allocate(free_list);
      return p;
    }
  }

  // Create a new allocator list, borrowing from an allocator with space
  _Borrow(nullptr, tid, true);

  return kNullPointer;
}

Pointer PageAllocator::Reallocate(Pointer &ptr, size_t new_size) {
  if (new_size <= header_->page_size_) {
    return ptr;
  } else {
    throw PAGE_SIZE_UNSUPPORTED.format(new_size);
  }
}

void PageAllocator::Free(Pointer &ptr) {
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto free_list_id = (tid + i) % header_->concurrency_;
    auto &free_list = free_lists_[free_list_id];
    auto list_lock = ScopedMutex(free_list.lock_);
    if (list_lock.TryLock()) {
      _Free(&free_list, ptr);
      return;
    }
  }

  auto free_list_id = tid % header_->concurrency_;
  auto &free_list = free_lists_[free_list_id];
  auto list_lock = ScopedMutex(free_list.lock_);
  list_lock.Lock();
  _Free(&free_list, ptr);
}

Pointer PageAllocator::_Allocate(PageFreeList &free_list) {
  // Dequeue a page from the free list
  _queue<Page> q;
  q.Attach(&free_list.queue_, slot_.ptr_);
  if (q.size()) {
    size_t off = q.dequeue_off();
    free_list.free_size_ -= header_->page_size_;
    free_list.alloc_size_ += header_->page_size_;
    return Pointer(GetId(), off);
  }

  // Create a new page from the segment
  if (free_list.region_size_ >= header_->page_size_) {
    size_t off = free_list.region_off_;
    free_list.region_size_ -= header_->page_size_;
    free_list.region_off_ += header_->page_size_;
    free_list.free_size_ -= header_->page_size_;
    free_list.alloc_size_ += header_->page_size_;
    return Pointer(GetId(), off);
  }

  return kNullPointer;
}

void PageAllocator::_Borrow(PageFreeList *to, tid_t tid, bool append) {
  tid += 1;
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto free_list_id = (tid + i) % header_->concurrency_;
    auto &from = free_lists_[free_list_id];
    auto list_lock = ScopedMutex(from.lock_);
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
  q.Attach(&free_list->queue_, slot_.ptr_);
  q.enqueue_off(p.off_);
  free_list->free_size_ += header_->page_size_;
  free_list->alloc_size_ -= header_->page_size_;
}

}  // namespace labstor::ipc