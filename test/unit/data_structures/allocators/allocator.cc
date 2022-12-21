/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#include "test_init.h"

void PageAllocationTest(Allocator *alloc) {
  int count = 1024;
  size_t page_size = KILOBYTES(4);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;

  // Allocate pages
  std::vector<Pointer> ps(count);
  void *ptrs[count];
  for (int i = 0; i < count; ++i) {
    ptrs[i] = alloc->AllocatePtr<void>(page_size, ps[i]);
    memset(ptrs[i], i, page_size);
    REQUIRE(ps[i].off_ != 0);
    REQUIRE(!ps[i].is_null());
    REQUIRE(ptrs[i] != nullptr);
  }

  // Convert process pointers into independent pointers
  for (int i = 0; i < count; ++i) {
    Pointer p = mem_mngr->Convert(ptrs[i]);
    REQUIRE(p == ps[i]);
    REQUIRE(VerifyBuffer((char*)ptrs[i], page_size, i));
  }

  // Check the custom header
  auto hdr = alloc->GetCustomHeader<SimpleAllocatorHeader>();
  REQUIRE(hdr->checksum_ == HEADER_CHECKSUM);

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
