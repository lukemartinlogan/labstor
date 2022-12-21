//
// Created by lukemartinlogan on 12/21/22.
//

#include <malloc.h>
#include <stdlib.h>
#include "labstor/memory/memory_manager.h"

using labstor::ipc::Pointer;
using labstor::ipc::Allocator;

/** Allocate SIZE bytes of memory. */
void* malloc(size_t size) {
  auto alloc = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
  return alloc->AllocatePtr<void>(size);
}

/** Allocate NMEMB elements of SIZE bytes each, all initialized to 0. */
void* calloc(size_t nmemb, size_t size) {
  auto alloc = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
  return alloc->ClearAllocatePtr<void>(nmemb * size);
}

/**
 * Re-allocate the previously allocated block in ptr, making the new
 * block SIZE bytes long.
 * */
void* realloc(void *ptr, size_t size) {
  Pointer p = LABSTOR_MEMORY_MANAGER->Convert(ptr);
  auto alloc = LABSTOR_MEMORY_MANAGER->GetAllocator(p.allocator_id_);
  return alloc->AllocatePtr<void>(size);
}

/**
 * Re-allocate the previously allocated block in PTR, making the new
 * block large enough for NMEMB elements of SIZE bytes each.
 * */
void* reallocarray(void *ptr, size_t nmemb, size_t size) {
  return realloc(ptr, nmemb * size);
}

/** Free a block allocated by `malloc', `realloc' or `calloc'. */
void free(void *ptr) {
  Pointer p = LABSTOR_MEMORY_MANAGER->Convert(ptr);
  auto alloc = LABSTOR_MEMORY_MANAGER->GetAllocator(p.allocator_id_);
  alloc->Free(p);
}

/** Allocate SIZE bytes allocated to ALIGNMENT bytes. */
void* memalign(size_t alignment, size_t size) {
  // TODO(llogan): need to add an aligned allocator
  return malloc(size);
}

/** Allocate SIZE bytes on a page boundary. */
void* valloc(size_t size) {
  return memalign(LABSTOR_SYSTEM_INFO->page_size_, size);
}

/** The nearest multiple of the alignment */
size_t NextMultiple(size_t alignment, size_t size) {
  auto page_size = LABSTOR_SYSTEM_INFO->page_size_;
  size_t new_size = size;
  size_t page_off = size % alignment;
  if (page_off) {
    new_size = size + page_size - page_off;
  }
  return new_size;
}

/**
 * Equivalent to valloc(minimum-page-that-holds(n)),
 * that is, round up size to nearest pagesize.
 * */
void* pvalloc(size_t size) {
  auto page_size = LABSTOR_SYSTEM_INFO->page_size_;
  size_t new_size = NextMultiple(page_size, size);
  return valloc(new_size);
}

/**
 * Allocates size bytes and places the address of the
 * allocated memory in *memptr. The address of the allocated memory
 * will be a multiple of alignment, which must be a power of two and a multiple
 * of sizeof(void *). Returns NULL if size is 0. */
int posix_memalign(void **memptr, size_t alignment, size_t size) {
  (*memptr) = memalign(alignment, size);
}

/**
 * Aligned to an alignment with a size that is a multiple of the
 * alignment
 * */
void *aligned_alloc(size_t alignment, size_t size) {
  return memalign(alignment, NextMultiple(alignment, size));
}
