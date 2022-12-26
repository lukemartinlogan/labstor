//
// Created by lukemartinlogan on 12/25/22.
//

#include "labstor/memory/allocator/multi_page_allocator.h"

namespace labstor::ipc {

/**
 * base^y = x; solve for y
 * */
size_t CountLogarithm(RealNumber base, size_t x, size_t &round) {
  size_t y = 1;
  RealNumber compound(base);
  while (compound.as_int() < x) {
    compound *= base;
    RealNumber compound_exp = compound * compound;
    y += 1;
    if (compound_exp.as_int() <= x) {
      y *= y;
      compound = compound_exp;
    }
  }
  round = compound.as_int();
  return y - 1;
}

/**
 * The index of the free list to allocate pages from
 * */
inline size_t MultiPageAllocator::IndexLogarithm(size_t x, size_t &round) {
  if (x <= header_->min_page_size_) {
    return 0;
  } else if (x > header_->max_page_size_){
    return header_->max_page_idx_ - header_->min_page_idx_ + 1;
  }
  return CountLogarithm(header_->growth_rate_, x, round) -
  header_->min_page_idx_;
}

/**
   * Initialize the allocator in shared memory
   * */
void MultiPageAllocator::shm_init(MemoryBackend *backend,
                                  allocator_id_t alloc_id,
                                  size_t custom_header_size,
                                  size_t min_page_size,
                                  size_t max_page_size,
                                  RealNumber growth_rate,
                                  size_t coalesce_min_size,
                                  RealNumber coalesce_frac,
                                  size_t thread_table_size,
                                  int concurrency) {
  backend_ = backend;
  header_ = reinterpret_cast<MultiPageAllocatorHeader*>(backend_->data_);
  header_->Configure(alloc_id, custom_header_size, min_page_size,
                     max_page_size, growth_rate, coalesce_min_size,
                     coalesce_frac, thread_table_size, concurrency);
  custom_header_ = GetCustomHeader<char>();
  // Get the number of page sizes to cache per MultiPageFreeList
  size_t round;
  header_->min_page_idx_ = CountLogarithm(growth_rate, min_page_size, round);
  header_->max_page_idx_ = CountLogarithm(growth_rate, max_page_size, round);
  size_t num_cached_pages = header_->max_page_idx_ - header_->min_page_idx_;
  num_cached_pages += 1;
  // Get the size of a single MultiPageFreeList
  size_t mp_free_list_elmt_size = _array<MultiPageFreeList>::GetSizeBytes(
    1, MultiPageFreeList::GetSizeBytes(num_cached_pages + 1));
  // Allocate the array of MultiPageFreeLists
  void *free_list_start = GetFreeListStart();
  mp_free_lists_.shm_init(free_list_start,
                          header_->thread_table_size_,
                          mp_free_list_elmt_size);
  mp_free_lists_.resize(concurrency);
  // Initialize each free list with an equal segment of the backend
  size_t cur_off = mp_free_lists_.After() - backend_->data_;
  size_t cur_size = backend_->data_size_ - cur_off;
  size_t per_conc_region = cur_size / header_->concurrency_;
  for (auto &mp_free_list : mp_free_lists_) {
    mp_free_list.shm_init(cur_off, per_conc_region);
    cur_off += per_conc_region;
  }
}

/**
 * Attach an existing allocator from shared memory
 * */
void MultiPageAllocator::shm_deserialize(MemoryBackend *backend) {
  backend_ = backend;
  header_ = reinterpret_cast<MultiPageAllocatorHeader*>(backend_->data_);
  custom_header_ = GetCustomHeader<char>();
  void *free_list_start = GetFreeListStart();
  mp_free_lists_.shm_deserialize(free_list_start);
}

/**
 * Allocate a memory of \a size size.
 * */
Pointer MultiPageAllocator::Allocate(size_t size) {
  size_t page_size;
  size_t page_size_idx = IndexLogarithm(size, page_size);

  // Get current thread ID
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();

  // Allocate without lock if possible
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto mp_free_list_id = (tid + i) % header_->concurrency_;
    auto &mp_free_list = mp_free_lists_[mp_free_list_id];
    ScopedMutex list_lock(mp_free_list.lock_);
    if (mp_free_list.free_size_ < size) continue;
    if (list_lock.TryLock()) {
      Pointer p = _Allocate(mp_free_list, page_size_idx, page_size);
      if (!p.is_null()) return p;
      _Borrow(&mp_free_list, tid, false);
      p = _Allocate(mp_free_list, page_size_idx, page_size);
      return p;
    }
  }

  // Create a new allocator list, borrowing from an allocator with space
  _Borrow(nullptr, tid, true);

  return kNullPointer;
}

/**
 * Allocate a memory of \a size size, which is aligned to \a
 * alignment.
 * */
Pointer MultiPageAllocator::AlignedAllocate(size_t size, size_t alignment) {
}

/**
 * Reallocate \a p pointer to \a new_size new size.
 *
 * @return whether or not the pointer p was changed
 * */
bool MultiPageAllocator::ReallocateNoNullCheck(Pointer &p, size_t new_size) {
}

/**
 * Free \a ptr pointer. Null check is performed elsewhere.
 * */
void MultiPageAllocator::FreeNoNullCheck(Pointer &ptr) {
}

/**
 * Get the current amount of data allocated. Can be used for leak
 * checking.
 * */
size_t MultiPageAllocator::GetCurrentlyAllocatedSize() {
  size_t total_alloced = 0;
  size_t total_freed = 0;
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto &mp_free_list = mp_free_lists_[i];
    total_alloced += mp_free_list.total_alloced_;
    total_freed += mp_free_list.total_freed_;
  }
  return total_alloced - total_freed;
}

void* MultiPageAllocator::GetFreeListStart() {
  return reinterpret_cast<void*>(
    custom_header_ + header_->custom_header_size_);
}

Pointer MultiPageAllocator::_Allocate(MultiPageFreeList &mp_free_list,
                                      size_t page_size_idx,
                                      size_t page_size) {
  _queue<MpPage> page_free_list;
  // Each page has a header with info needed for aligned allocations
  page_size += sizeof(MpPageHeader);

  // Re-use a cached page
  mp_free_list.GetPageFreeList(page_size_idx, backend_->data_, page_free_list);
  if (page_free_list.size()) {
    size_t off = page_free_list.dequeue_off();
    mp_free_list.free_size_ -= page_size;
    mp_free_list.total_alloced_ += page_size;
    return Pointer(GetId(), off);
  }

  // Create a new page from the segment
  if (mp_free_list.region_size_ >= page_size) {
    size_t off = mp_free_list.region_off_;
    mp_free_list.region_size_ -= page_size;
    mp_free_list.region_off_ += page_size;
    mp_free_list.free_size_ -= page_size;
    mp_free_list.total_alloced_ += page_size;
    return Pointer(GetId(), off);
  }

  return kNullPointer;
}

void MultiPageAllocator::_Borrow(MultiPageFreeList *to,
                                 tid_t tid, bool append) {
}

void MultiPageAllocator::_Free(MultiPageFreeList *free_list, Pointer &p) {
}

}  // namespcae labstor::ipc