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

#include "basic_test.h"
#include "test_init.h"
#include <labstor/data_structures/unordered_map.h>
#include <labstor/data_structures/lockless/string.h>
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;
using labstor::ipc::unordered_map;
using labstor::ipc::lockless::string;

void UnorderedMapOfIntTest() {
  Allocator *alloc = alloc_g;
  unordered_map<int, int> map(alloc);
}

void UnorderedMapOfStringTest() {
}

TEST_CASE("UnorderedMapOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfIntTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("UnorderedMapOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
