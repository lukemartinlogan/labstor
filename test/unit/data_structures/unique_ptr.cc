//
// Created by lukemartinlogan on 12/11/22.
//

#include "labstor/data_structures/unique_ptr.h"
#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::string;
using labstor::ipc::unique_ptr;

void UniquePtrOfInt() {
  Allocator *alloc = alloc_g;
  unique_ptr<int> hello(alloc, 25);
}

void UniquePtrOfString() {
  Allocator *alloc = alloc_g;
  unique_ptr<string> text(alloc, "there", alloc);
}

TEST_CASE("UniquePtrOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UniquePtrOfInt();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("UniquePtrOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UniquePtrOfString();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
