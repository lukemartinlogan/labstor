//
// Created by lukemartinlogan on 12/11/22.
//

#include "labstor/data_structures/manual_ptr.h"
#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::string;
using labstor::ipc::mptr;
using labstor::ipc::uptr;
using labstor::ipc::mptr;
using labstor::ipc::ShmArchive;

void ManualPtrOfInt() {
  Allocator *alloc = alloc_g;
  mptr<int> data(25);
  int x = data.get_ref();
  REQUIRE(data.get_ref() == 25);
  REQUIRE(data.get_ref_const() == 25);
  REQUIRE(*data == 25);
  // TestCopy(hello);

  mptr<int> data2 = std::move(data);
  REQUIRE(data.IsNull());
  REQUIRE(std::hash<mptr<int>>{}(data2) == std::hash<int>{}(25));

  /*ShmArchive<mptr<int>> ar;
  data2 >> ar;
  mptr<int> from_ar(ar);
  REQUIRE(*from_ar == 25);*/
}

void ManualPtrOfString() {
  Allocator *alloc = alloc_g;
  mptr<string> data(alloc, "there");
  REQUIRE(data->str() == "there");
  REQUIRE((*data).str() == "there");
  mptr<string> data2 = std::move(data);

  ShmArchive<mptr<string>> ar;
  data2 >> ar;
  mptr<string> from_ar(ar);
  REQUIRE(*from_ar == "there");

  data.shm_destroy();
}

TEST_CASE("ManualPtrOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ManualPtrOfInt();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("ManualPtrOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ManualPtrOfString();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
