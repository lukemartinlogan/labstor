//
// Created by lukemartinlogan on 12/11/22.
//

#include "labstor/data_structures/unique_ptr.h"
#include "labstor/data_structures/manual_ptr.h"
#include "labstor/data_structures/shm_ref.h"
#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::string;
using labstor::ipc::unique_ptr;
using labstor::ipc::make_uptr;
using labstor::ipc::uptr;
using labstor::ipc::mptr;
using labstor::ipc::ShmArchive;

void CopyConstructorShouldFail(unique_ptr<int> num) {
  std::cout << "Probably shouldn't work" << std::endl;
}

void UniquePtrOfInt() {
  Allocator *alloc = alloc_g;
  uptr<int> data = make_uptr<int>(25);
  REQUIRE(data.get_ref() == 25);
  REQUIRE(data.get_ref_const() == 25);
  REQUIRE(*data == 25);
  // TestCopy(hello);

  uptr<int> data2 = std::move(data);
  REQUIRE(data.IsNull());
  REQUIRE(std::hash<uptr<int>>{}(data2) == std::hash<int>{}(25));

  ShmArchive<uptr<int>> ar;
  data2 >> ar;
  mptr<int> from_ar(ar);
  REQUIRE(*from_ar == 25);
}

void UniquePtrOfString() {
  Allocator *alloc = alloc_g;
  unique_ptr<string> data = make_uptr<string>(alloc, "there");
  REQUIRE(data->str() == "there");
  REQUIRE((*data).str() == "there");
  unique_ptr<string> data2 = std::move(data);

  ShmArchive<unique_ptr<string>> ar;
  data2 >> ar;
  mptr<string> from_ar(ar);
  REQUIRE(*from_ar == "there");
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
