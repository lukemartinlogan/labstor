//
// Created by lukemartinlogan on 12/25/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_MULTI_PAGE_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_MULTI_PAGE_ALLOCATOR_H_

#include "allocator.h"
#include "labstor/thread/lock.h"
#include "labstor/data_structures/thread_unsafe/_queue.h"
#include "page_allocator.h"

namespace labstor::ipc {

struct MpPage : public _queue_entry {
  size_t page_size_;  /**< The size of the page allocated */
  uint32_t off_;        /**< The offset within the page */
  uint32_t page_idx_;   /**< The id of the page in the mp free list */
};

struct MultiPageFreeList {
  /// Lock the list
  Mutex lock_;
  /// Free list for different page sizes
  _array_header<_queue_header<MpPage>> free_lists_;
  /// Stack allocator
  size_t region_off_, region_size_;
  /// Number of bytes currently free in this free list
  size_t free_size_;
  /// Total number of bytes alloc'd from this list
  size_t total_alloced_;
  /// Total number of bytes freed to this list
  size_t total_freed_;

  /**
   * The total amount of space allocated by the MultiPageFree list when
   * there is \a num_page_caches number of page size free lists
   * */
  static size_t GetSizeBytes(size_t num_page_caches) {
    return sizeof(MultiPageFreeList) +
    _array<_queue<Page>>::GetSizeBytes(
      num_page_caches, sizeof(_queue_header<Page>));
  }

  /**
   * Initialize the free list array
   * */
  void shm_init(size_t mp_free_list_elmt_size,
                char *region_start,
                size_t region_off, size_t region_size) {
    Allocator::ConstructObj<MultiPageFreeList>(*this);
    void *after = reinterpret_cast<void*>(this + 1);
    _array<_queue_header<MpPage>> free_lists;
    free_lists.shm_init(after,
                        mp_free_list_elmt_size - sizeof(MultiPageFreeList));
    free_lists.resize_full();
    for (auto &qh : free_lists) {
      _queue<MpPage> q;
      q.shm_init(&qh, region_start);
    }
    region_off_= region_off;
    region_size_ = region_size;
    free_size_ = region_size;
    total_alloced_ = 0;
    total_freed_ = 0;
  }

  /** Get the free list at index i */
  void GetPageFreeList(size_t i, char *region_start, _queue<MpPage> &page_free_list) {
    void *after = reinterpret_cast<void*>(this + 1);
    _array<_queue_header<MpPage>> free_lists;
    free_lists.shm_deserialize(after);
    _queue_header<MpPage> &hdr = free_lists[i];
    page_free_list.shm_deserialize(&hdr, region_start);
  }
};

struct MultiPageAllocatorHeader : public AllocatorHeader {
  /// Number of threads to initially assume
  std::atomic<uint32_t> concurrency_;
  /// Bytes to dedicate to per-thread free list tables
  size_t thread_table_size_;
  /// Cache every page between these sizes
  size_t mp_free_list_size_;
  size_t min_page_size_, max_page_size_;
  uint32_t min_page_log_, max_page_log_, last_page_idx_;
  /// The page sizes to cache
  RealNumber growth_rate_;
  /// The minimum number of free bytes before a coalesce can be triggered
  size_t coalesce_min_size_;
  /// The percentage of fragmentation before a coalesce is triggered
  RealNumber coalesce_frac_;

  MultiPageAllocatorHeader() = default;

  void Configure(allocator_id_t alloc_id,
                 size_t custom_header_size,
                 size_t min_page_size,
                 size_t max_page_size,
                 RealNumber growth_rate,
                 size_t coalesce_min_size,
                 RealNumber coalesce_frac,
                 size_t thread_table_size,
                 uint32_t concurrency) {
    AllocatorHeader::Configure(alloc_id, AllocatorType::kMultiPageAllocator,
                               custom_header_size);
    min_page_size_ = min_page_size;
    max_page_size_ = max_page_size;
    growth_rate_ = growth_rate;
    coalesce_min_size_ = coalesce_min_size;
    coalesce_frac_ = coalesce_frac;
    thread_table_size_ = thread_table_size;
    concurrency_ = concurrency;
  }
};

class MultiPageAllocator : public Allocator {
 private:
  MultiPageAllocatorHeader *header_;
  char *custom_header_;
  _array<MultiPageFreeList> mp_free_lists_;

 public:
  /**
   * Allocator constructor
   * */
  MultiPageAllocator()
    : header_(nullptr), custom_header_(nullptr) {}

  /**
   * Determine the size of the shared-memory header
   * */
  size_t GetInternalHeaderSize() override {
    return sizeof(MultiPageAllocatorHeader);
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
                allocator_id_t alloc_id,
                size_t custom_header_size = 0,
                size_t min_page_size = 64,
                size_t max_page_size = KILOBYTES(32),
                RealNumber growth_rate = RealNumber(5, 4),
                size_t coalesce_min_size = MEGABYTES(20),
                RealNumber coalesce_frac = RealNumber(2, 1),
                size_t thread_table_size = MEGABYTES(1),
                uint32_t concurrency = 4);

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
  /** Get the index of "x" within the page free list array */
  inline uint32_t IndexLogarithm(size_t x, size_t &round);

  /** Get the pointer to the mp_free_lists_ array */
  void* GetMpFreeListStart();

  /** Set the page's header + change allocator stats*/
  void _AllocateHeader(Pointer &p,
                       MultiPageFreeList &mp_free_list,
                       size_t page_size,
                       size_t page_size_idx,
                       size_t off);

  /** Allocate a page from a thread's mp free list */
  Pointer _Allocate(MultiPageFreeList &free_list,
                    size_t page_size_idx, size_t page_size);

  /** Allocate a large, cached page */
  bool _AllocateLargeCached(MultiPageFreeList &mp_free_list,
                            size_t page_size_idx,
                            size_t page_size,
                            Pointer &ret);

  /** Allocate a cached page */
  bool _AllocateCached(MultiPageFreeList &mp_free_list,
                       size_t page_size_idx,
                       size_t page_size,
                       Pointer &ret);

  /** Allocate and divide a cached page larger than page_size */
  bool _AllocateBorrowCached(MultiPageFreeList &mp_free_list,
                             size_t page_size_idx,
                             size_t page_size,
                             Pointer &ret);

  /** Allocate a page from the segment */
  bool _AllocateSegment(MultiPageFreeList &mp_free_list,
                        size_t page_size_idx,
                        size_t page_size,
                        Pointer &ret);

  /** Reorganize free space to minimize fragmentation */
  void _Coalesce(MultiPageFreeList &to, tid_t tid);

  /** Create a new thread allocator by borrowing from other allocators */
  void _AddThread();

  /** Free a page to a free list */
  void _Free(MultiPageFreeList &free_list, Pointer &p);
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_MULTI_PAGE_ALLOCATOR_H_
