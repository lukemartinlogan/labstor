//
// Created by lukemartinlogan on 12/25/22.
//

#include "labstor/memory/allocator/multi_page_allocator.h"

namespace labstor::ipc {

/**
 * base^y = x; solve for y (round up)
 * */
uint32_t CountLogarithm(RealNumber base, size_t x, size_t &round) {
  uint32_t y = 1;
  RealNumber compound(base);
  while (compound.as_int() < x) {
    compound *= base;
    RealNumber compound_exp = compound * compound;
    y += 1;
    if (compound_exp.as_int() <= x) {
      y *= 2;
      compound = compound_exp;
    }
  }
  round = compound.as_int();
  return y;
}

/**
 * The index of the free list to allocate pages from
 * */
inline uint32_t MultiPageAllocator::IndexLogarithm(size_t x, size_t &round) {
  if (x <= header_->min_page_size_) {
    round = header_->min_page_size_;
    return 0;
  } else if (x > header_->max_page_size_){
    round = x;
    return header_->last_page_idx_;
  }
  size_t log = CountLogarithm(header_->growth_rate_, x, round);
  return log - header_->min_page_log_;
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
                                  uint32_t concurrency) {
  backend_ = backend;
  header_ = reinterpret_cast<MultiPageAllocatorHeader*>(backend_->data_);
  header_->Configure(alloc_id, custom_header_size, min_page_size,
                     max_page_size, growth_rate, coalesce_min_size,
                     coalesce_frac, thread_table_size, concurrency);
  custom_header_ = GetCustomHeader<char>();
  // Get the number of page sizes to cache per MultiPageFreeList
  size_t round;
  header_->min_page_log_ = CountLogarithm(growth_rate, min_page_size, round);
  header_->max_page_log_ = CountLogarithm(growth_rate, max_page_size, round);
  header_->last_page_idx_ = header_->max_page_log_ - header_->min_page_log_ + 1;
  size_t num_cached_pages = header_->last_page_idx_ + 1;
  // Get the size of a single MultiPageFreeList
  header_->mp_free_list_size_ = _array<MultiPageFreeList>::GetSizeBytes(
    1, MultiPageFreeList::GetSizeBytes(num_cached_pages));
  // Allocate the array of MultiPageFreeLists
  void *free_list_start = GetMpFreeListStart();
  mp_free_lists_.shm_init(free_list_start,
                          header_->thread_table_size_,
                          header_->mp_free_list_size_);
  mp_free_lists_.resize_full();
  // Initialize each free list with an equal segment of the backend
  size_t cur_off = mp_free_lists_.After() - backend_->data_;
  size_t cur_size = backend_->data_size_ - cur_off;
  size_t per_conc_region = cur_size / header_->concurrency_;
  for (size_t i = 0; i < header_->concurrency_; ++i) {
    auto &mp_free_list = mp_free_lists_[i];
    mp_free_list.shm_init(header_->mp_free_list_size_,
                          backend->data_,
                          cur_off, per_conc_region);
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
  void *free_list_start = GetMpFreeListStart();
  mp_free_lists_.shm_deserialize(free_list_start);
}

/**
 * Sets the header
 * */
void MultiPageAllocator::_AllocateHeader(Pointer &p,
                                         size_t page_size,
                                         size_t page_size_idx,
                                         size_t off) {
  auto hdr = Convert<MpPage>(p);
  hdr->SetAllocated();
  hdr->page_size_ = page_size;
  hdr->page_idx_ = page_size_idx;
  hdr->off_ = off;
}

/**
 * Sets the header and decrements counters
 * */
void MultiPageAllocator::_AllocateHeader(Pointer &p,
                                         MultiPageFreeList &mp_free_list,
                                         size_t page_size,
                                         size_t page_size_idx,
                                         size_t off) {
  _AllocateHeader(p, page_size, page_size_idx, off);
  mp_free_list.free_size_ -= page_size;
  mp_free_list.total_alloced_ += page_size;
}

/**
 * Allocate a memory of \a size size.
 * */
Pointer MultiPageAllocator::Allocate(size_t size) {
  size_t page_size;
  uint32_t page_size_idx = IndexLogarithm(size + sizeof(MpPage),
                                          page_size);
  bool all_locks_held = true;

  // Get current thread ID
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();

  // Allocate without lock if possible
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto mp_free_list_id = (tid + i) % header_->concurrency_;
    auto &mp_free_list = mp_free_lists_[mp_free_list_id];
    ScopedMutex list_lock(mp_free_list.lock_);
    if (list_lock.TryLock()) {
      all_locks_held = false;
      if (mp_free_list.free_size_ < page_size) continue;
      Pointer p = _Allocate(mp_free_list, page_size_idx, page_size);
      if (p.is_null()) { continue; }
      _AllocateHeader(p, mp_free_list,
                      page_size, page_size_idx, sizeof(MpPage));
      p += sizeof(MpPage);
      return p;
    }
  }

  // Create a new allocator list to cache
  if (all_locks_held) {
    _AddThread();
  }

  // Allocate from first list with space
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto mp_free_list_id = (tid + i) % header_->concurrency_;
    auto &mp_free_list = mp_free_lists_[mp_free_list_id];
    ScopedMutex list_lock(mp_free_list.lock_);
    if (mp_free_list.free_size_ < size) continue;
    list_lock.Lock();
    Pointer p = _Allocate(mp_free_list, page_size_idx, page_size);
    if (p.is_null()) { continue; }
    _AllocateHeader(p, mp_free_list,
                    page_size, page_size_idx, sizeof(MpPage));
    p += sizeof(MpPage);
    return p;
  }

  return kNullPointer;
}

/**
 * Allocate a memory of \a size size, which is aligned to \a
 * alignment.
 * */
Pointer MultiPageAllocator::AlignedAllocate(size_t size, size_t alignment) {
  // Add alignment to size to guarantee the ability to shift
  size += alignment;
  Pointer p = Allocate(size);
  if (p.is_null()) { return p; }
  // Get the current header statistics
  auto hdr = Convert<MpPage>(p - sizeof(MpPage));
  size_t page_idx = hdr->page_idx_;
  size_t page_size = hdr->page_size_;
  // Get the alignment offset
  auto ptr = Convert<char>(p);
  size_t align_off = reinterpret_cast<size_t>(ptr) % alignment;
  size_t new_page_off = alignment - align_off;
  p += new_page_off - sizeof(MpPage);
  // Create the header for the aligned payload
  _AllocateHeader(p, page_size, page_idx, new_page_off);
  return p + sizeof(MpPage);
}

/**
 * Reallocate \a p pointer to \a new_size new size.
 *
 * @return whether or not the pointer p was changed
 * */
bool MultiPageAllocator::ReallocateNoNullCheck(Pointer &p, size_t new_size) {
  auto hdr = Convert<MpPage>(p - sizeof(MpPage));
  size_t cur_size = hdr->page_size_ - hdr->off_;
  if (new_size <= cur_size) {
    return false;
  }
  Pointer new_p = Allocate(new_size);
  char *src = Convert<char>(p);
  char *dst = Convert<char>(new_p);
  memcpy(dst, src, cur_size);
  Free(p);
  p = new_p;
  return true;
}

/**
 * Free \a ptr pointer. Null check is performed elsewhere.
 * */
void MultiPageAllocator::FreeNoNullCheck(Pointer &p) {
  auto thread_info = LABSTOR_THREAD_MANAGER->GetThreadStatic();
  auto tid = thread_info->GetTid();

  // Try to find an unlocked list to free to
  for (auto i = 0; i < header_->concurrency_; ++i) {
    auto mp_free_list_id = (tid + i) % header_->concurrency_;
    auto &mp_free_list = mp_free_lists_[mp_free_list_id];
    ScopedMutex list_lock(mp_free_list.lock_);
    if (list_lock.TryLock()) {
      _Free(mp_free_list, p);
      return;
    }
  }

  // Force lock the list the thread corresponds to
  auto free_list_id = tid % header_->concurrency_;
  auto &mp_free_list = mp_free_lists_[free_list_id];
  ScopedMutex list_lock(mp_free_list.lock_);
  list_lock.Lock();
  _Free(mp_free_list, p);
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

/** Get the pointer to the mp_free_lists_ array */
void* MultiPageAllocator::GetMpFreeListStart() {
  return reinterpret_cast<void*>(
    custom_header_ + header_->custom_header_size_);
}

/** Allocate a page from a thread's mp free list */
Pointer MultiPageAllocator::_Allocate(MultiPageFreeList &mp_free_list,
                                      size_t page_size_idx,
                                      size_t page_size) {
  Pointer ret;

  // Re-use a cached page
  if (page_size_idx < header_->last_page_idx_) {
    if (_AllocateCached(mp_free_list, page_size_idx, page_size, ret)) {
      return ret;
    } else if (_AllocateBorrowCached(mp_free_list, page_size_idx,
                                     page_size, ret)) {
      return ret;
    }
  } else {
    if (_AllocateLargeCached(mp_free_list, page_size_idx, page_size, ret)) {
      return ret;
    }
  }

  // TODO(llogan): check fragmentation

  // Create a new page from the thread's segment
  if (_AllocateSegment(mp_free_list, page_size_idx, page_size, ret)) {
    return ret;
  }

  return kNullPointer;
}

/** Allocate a large, cached page */
bool MultiPageAllocator::_AllocateLargeCached(MultiPageFreeList &mp_free_list,
                                              size_t page_size_idx,
                                              size_t page_size,
                                              Pointer &ret) {
  _queue<MpPage> page_free_list;
  mp_free_list.GetPageFreeList(page_size_idx, backend_->data_, page_free_list);
  for (size_t i = 0; i < page_free_list.size(); ++i) {
    size_t backend_off = page_free_list.dequeue_off();
    ret = Pointer(GetId(), backend_off);
    auto hdr = Convert<MpPage>(ret);
    if (hdr->page_size_ >= page_size) {
      return true;
    }
    page_free_list.enqueue_off(backend_off);
  }
  return false;
}

/** Allocate a cached page */
bool MultiPageAllocator::_AllocateCached(MultiPageFreeList &mp_free_list,
                                         size_t page_size_idx,
                                         size_t page_size,
                                         Pointer &ret) {
  _queue<MpPage> page_free_list;
  mp_free_list.GetPageFreeList(page_size_idx, backend_->data_, page_free_list);
  if (page_free_list.size() == 0) {
    return false;
  }
  size_t off = page_free_list.dequeue_off();
  ret = Pointer(GetId(), off);
  return true;
}

/** Allocate a larger cached page */
bool MultiPageAllocator::_AllocateBorrowCached(MultiPageFreeList &mp_free_list,
                                               size_t page_size_idx,
                                               size_t page_size,
                                               Pointer &ret) {
  // Get the free list corresponding to the original page_size
  size_t orig_page_size = page_size;
  _queue<MpPage> orig_page_free_list;
  mp_free_list.GetPageFreeList(page_size_idx, backend_->data_,
                               orig_page_free_list);
  // Find the first cached page larger than page_size
  do {
    _queue<MpPage> page_free_list;
    mp_free_list.GetPageFreeList(page_size_idx, backend_->data_, page_free_list);
    if (page_free_list.size() == 0) {
      page_size_idx += 1;
      page_size = (header_->growth_rate_ * page_size).as_int();
      continue;
    }
    // Dequeue the large page
    size_t backend_off = page_free_list.dequeue_off();
    auto hdr = Convert<MpPage>(Pointer(GetId(), backend_off));
    page_size = hdr->page_size_;
    // Shard the large page into original page sizes
    size_t num_shards = page_size / orig_page_size;
    size_t shard_off = page_size % orig_page_size;
    if (shard_off != 0) {
      num_shards -= 1;
    }
    for (int i = 0; i < num_shards; ++i) {
      Pointer p(GetId(), backend_off + orig_page_size * i);
      auto hdr = Convert<MpPage>(p);
      hdr->page_size_ = orig_page_size;
      hdr->off_ = 0;
      hdr->page_idx_ = page_size_idx;
      orig_page_free_list.enqueue_off(p.off_);
    }
    if (shard_off != 0) {
      Pointer p(GetId(), backend_off + orig_page_size * num_shards);
      auto hdr = Convert<MpPage>(p);
      hdr->page_size_ = orig_page_size + shard_off;
      hdr->off_ = 0;
      hdr->page_idx_ = page_size_idx;
      orig_page_free_list.enqueue_off(p.off_);
    }
    // Dequeue a page from the original page free list
    size_t off = orig_page_free_list.dequeue_off();
    ret = Pointer(GetId(), off);
    return true;
  } while(page_size_idx <= header_->last_page_idx_);

  return false;
}

/** Allocate a page off the thread's segment */
bool MultiPageAllocator::_AllocateSegment(MultiPageFreeList &mp_free_list,
                                          size_t page_size_idx,
                                          size_t page_size,
                                          Pointer &ret) {
  if (mp_free_list.region_size_ >= page_size) {
    size_t off = mp_free_list.region_off_;
    mp_free_list.region_off_ += page_size;
    mp_free_list.region_size_ -= page_size;
    ret = Pointer(GetId(), off);
    return true;
  }
  return false;
}

/** Create a new thread allocator by borrowing from other allocators */
void MultiPageAllocator::_AddThread() {
  bool ret;
  uint32_t expected;
  do {
    expected = header_->concurrency_.load();
    uint32_t desired = expected + 1;
    if (desired > mp_free_lists_.max_size()) {
      return;
    }
    ret = header_->concurrency_.compare_exchange_weak(
      expected,
      desired);
  } while (!ret);

  int new_tid = header_->concurrency_.fetch_add(1);
  auto &mp_free_list = mp_free_lists_[new_tid];
  mp_free_list.shm_init(header_->mp_free_list_size_, backend_->data_, 0, 0);
}

/** Free a page to a free list */
void MultiPageAllocator::_Free(MultiPageFreeList &mp_free_list, Pointer &p) {
  auto hdr = Convert<MpPage>(p - sizeof(MpPage));
  if (!hdr->IsAllocated()) {
    throw DOUBLE_FREE;
  }
  hdr->UnsetAllocated();
  _queue<MpPage> page_free_list;
  mp_free_list.GetPageFreeList(hdr->page_idx_,
                                backend_->data_, page_free_list);
  Pointer real_p = p - hdr->off_;
  page_free_list.enqueue_off(real_p.off_);
  mp_free_list.free_size_ += hdr->page_size_;
  mp_free_list.total_freed_ += hdr->page_size_;
}

}  // namespace labstor::ipc