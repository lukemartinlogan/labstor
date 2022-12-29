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


#include "labstor/data_structures/smart_ptr/manual_ptr.h"
#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include "labstor/memory/allocator/page_allocator.h"

using labstor::ipc::string;
using labstor::ipc::mptr;
using labstor::ipc::uptr;
using labstor::ipc::mptr;
using labstor::ipc::make_mptr;
using labstor::ipc::ShmArchive;

void ManualPtrOfInt() {
  Allocator *alloc = alloc_g;
  mptr<int> data = make_mptr<int>(25);
  int x = data.get_ref();
  REQUIRE(data.get_ref() == 25);
  REQUIRE(data.get_ref_const() == 25);
  REQUIRE(*data == 25);
  // TestCopy(hello);

  mptr<int> data2 = std::move(data);
  REQUIRE(data.IsNull());
  REQUIRE(std::hash<mptr<int>>{}(data2) == std::hash<int>{}(25));

  {
    ShmArchive<int> ar;
    data2 >> ar;
    REQUIRE(ar.header_ptr_ == data2.obj_.header_ptr_);
    mptr<int> from_ar(ar);
    REQUIRE(*from_ar == 25);
  }

  {
    ShmArchive<mptr<int>> ar;
    data2 >> ar;
    REQUIRE(ar.header_ptr_ == data2.obj_.header_ptr_);
    mptr<int> from_ar(ar);
    REQUIRE(*from_ar == 25);
  }

  data2.shm_destroy();
}

void ManualPtrOfString() {
  Allocator *alloc = alloc_g;
  mptr<string> data = make_mptr<string>(nullptr, alloc, "there");
  REQUIRE(data->str() == "there");
  REQUIRE((*data).str() == "there");
  mptr<string> data2 = std::move(data);

  ShmArchive<mptr<string>> ar;
  data2 >> ar;
  mptr<string> from_ar(ar);
  REQUIRE(*from_ar == "there");

  data2.shm_destroy();
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
  // ManualPtrOfString();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
