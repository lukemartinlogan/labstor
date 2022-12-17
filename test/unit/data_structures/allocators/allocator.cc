//
// Created by lukemartinlogan on 11/6/22.
//

#include "test_init.h"

void PageAllocationTest(Allocator *alloc) {
  int count = 1024;
  size_t page_size = KILOBYTES(4);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  // Allocate pages
  Pointer ps[count];
  void *ptrs[count];
  for (int i = 0; i < count; ++i) {
    ptrs[i] = alloc->AllocatePtr<void>(page_size, ps[i]);
    memset(ptrs[i], i, page_size);
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
    REQUIRE(ptrs[i] != nullptr);
  }

  //Convert process pointers into independent pointers
  for (int i = 0; i < count; ++i) {
    Pointer p = mem_mngr->Convert(ptrs[i]);
    REQUIRE(p == ps[i]);
    REQUIRE(VerifyBuffer((char*)ptrs[i], page_size, i));
  }

  // Free pages
  for (int i = 0; i < count; ++i) {
    alloc->Free(ps[i]);
  }

  // Reallocate pages
  for (int i = 0; i < count; ++i) {
    ptrs[i] = alloc->AllocatePtr<void>(page_size, ps[i]);
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
  }

  // Free again
  for (int i = 0; i < count; ++i) {
    alloc->Free(ps[i]);
  }
}

TEST_CASE("PageAllocator") {
  auto alloc = Pretest(AllocatorType::kPageAllocator);
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  PageAllocationTest(alloc);
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  Posttest();
}