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

void UnorderedMapOfIntTest() {
  Allocator *alloc = alloc_g;
  unordered_map<int, int> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      map.emplace(i, i);
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      REQUIRE(map[i] == i);
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      auto iter = map.find(i);
      auto entry = *iter;
      REQUIRE(entry.val_ == i);
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      REQUIRE((0 <= entry.key_ && entry.key_ < 20));
      REQUIRE((0 <= entry.val_ && entry.val_ < 20));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      map.erase(i);
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      REQUIRE(map.find(i) == map.end());
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (should force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      map.emplace(i, i);
    }
    for (int i = 0; i < 100; ++i) {
      REQUIRE(map.find(i) != map.end());
    }
  }

  map.shm_destroy();
}

void UnorderedMapOfStringTest() {
  Allocator *alloc = alloc_g;
  unordered_map<string, string> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      auto t1 = string(std::to_string(i));
      auto t2 = string(std::to_string(i + 1));
      map.emplace(t1, t2);
      t1.shm_destroy();
      t2.shm_destroy();
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      string t2(std::to_string(i + 1));
      auto t3 = map[t1];
      REQUIRE(t3 == t2);
      t1.shm_destroy();
      t2.shm_destroy();
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      string t2(std::to_string(i + 1));
      auto iter = map.find(t1);
      auto entry = *iter;
      REQUIRE(entry.val_ == t2);
      t1.shm_destroy();
      t2.shm_destroy();
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      int key;
      int val;
      std::stringstream(entry.key_.str()) >> key;
      std::stringstream(entry.val_.str()) >> val;
      REQUIRE((0 <= key && key < 20));
      REQUIRE((1 <= val && val < 21));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      map.erase(i_text);
      i_text.shm_destroy();
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) == map.end());
      i_text.shm_destroy();
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (will force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      map.emplace(i_text);
      i_text.shm_destroy();
    }
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) != map.end());
      i_text.shm_destroy();
    }
  }

  map.shm_destroy();
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
