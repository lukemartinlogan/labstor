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
using labstor::ipc::ShmArchive;

void CopyConstructorShouldFail(unique_ptr<int> num) {
  std::cout << "Probably shouldn't work" << std::endl;
}

void UniquePtrOfInt() {
  Allocator *alloc = alloc_g;
  unique_ptr<int> data(alloc, 25);
  REQUIRE(data.get() == 25);
  REQUIRE(*data == 25);
  // TestCopy(hello);

  unique_ptr<int> data2 = std::move(data);

  REQUIRE(std::hash<unique_ptr<int>>{}(data2) == std::hash<int>{}(25));

  ShmArchive<unique_ptr<int>> ar(data2);
  unique_ptr<int> from_ar(ar);
  REQUIRE(from_ar.owner_ == false);
  REQUIRE(data2.owner_ == true);
}

void UniquePtrOfString() {
  Allocator *alloc = alloc_g;
  unique_ptr<string> data(alloc, "there", alloc);
  REQUIRE(data.get().str() == "there");
  REQUIRE((*data).str() == "there");
  unique_ptr<string> data2 = std::move(data);

  ShmArchive<unique_ptr<string>> ar(data2);
  unique_ptr<string> from_ar(ar);
  REQUIRE(from_ar.owner_ == false);
  REQUIRE(data2.owner_ == true);
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
