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


#include "labstor/data_structures/smart_ptr/unique_ptr.h"
#include "labstor/data_structures/smart_ptr/manual_ptr.h"
#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/string.h"
#include "labstor/memory/allocator/page_allocator.h"
#include "smart_ptr.h"

using labstor::ipc::string;
using labstor::ipc::unique_ptr;
using labstor::ipc::make_uptr;
using labstor::ipc::uptr;
using labstor::ipc::mptr;
using labstor::ipc::TypedPointer;

template<typename T>
void UniquePtrTest() {
  Allocator *alloc = alloc_g;
  lipc::SmartPtrTestSuite<T, uptr<T>> test;
  CREATE_SET_VAR_TO_INT_OR_STRING(T, num, 25);
  test.ptr_ = make_uptr<T>(num);
  test.DereferenceTest(num);
  test.MoveConstructorTest(num);
  test.MoveAssignmentTest(num);
  test.SerializeationConstructorTest(num);
  test.SerializeationOperatorTest(num);
}

TEST_CASE("UniquePtrOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UniquePtrTest<lipc::string>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
