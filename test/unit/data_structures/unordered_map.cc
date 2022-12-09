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
#include "labstor/data_structures/thread_safe/unordered_map.h"
#include "labstor/data_structures/string.h"
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;
using labstor::ipc::unordered_map;
using labstor::ipc::string;

/**
 * TODO(llogan): Shared-memory data structure iterators take "this" as a
 * stored parameter. This results in problems in the unordered_map, as
 * we create vector and list objects for only the scope of the begin()
 * and end() functions.
 *
 * Do we need to load data structure from SHM every single iteration?
 * What if we store vector + list objects in the iterator itself?
 * Probably the only solution.
 * */

void UnorderedMapOfIntTest() {
  Allocator *alloc = alloc_g;
  unordered_map<int, int> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  for (int i = 0; i < 20; ++i) {
    map.emplace(i, i);
  }

  // Check if the 20 entries are indexable
  for (int i = 0; i < 20; ++i) {
    REQUIRE(map[i] == i);
  }

  // Check if 20 entries are findable
  for (int i = 0; i < 20; ++i) {
    auto iter = map.find(i);
    auto &entry = *iter;
    REQUIRE(entry.obj_ == i);
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto &entry : map) {
      std::cout << entry.obj_ << std::endl;
      REQUIRE((0 <= entry.key_ && entry.key_ < 20));
      REQUIRE((0 <= entry.obj_ && entry.obj_ < 20));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Remove 20 entries from the map
  for (int i = 0; i < 20; ++i) {
  }

  map.shm_destroy();
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
