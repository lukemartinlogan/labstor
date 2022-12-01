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
#include <labstor/data_structures/lockless/list.h>
#include <labstor/data_structures/lockless/string.h>
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;
using labstor::ipc::lockless::list;
using labstor::ipc::lockless::string;

void ListTest() {
  Allocator *alloc = alloc_g;
  list<int> lp(alloc);
  lp.shm_init();

  for (int i = 0; i < 30; ++i) {
    lp.emplace_back(i);
  }
  REQUIRE(lp.size() == 30);

  {
    int fcur = 0;
    for (auto num : lp) {
      REQUIRE(num == fcur);
      ++fcur;
    }
  }

  lp.emplace_front(100);
  REQUIRE(lp.front() == 100);
  REQUIRE(lp.size() == 31);

  lp.erase(lp.begin(), lp.begin() + 1);

  {
    int fcur = 0;
    for (auto num : lp) {
      REQUIRE(num == fcur);
      ++fcur;
    }
  }

  lp.erase(lp.begin(), lp.end());
  REQUIRE(lp.size() == 0);

  lp.shm_destroy();
}

void ListOfStringTest() {
  Allocator *alloc = alloc_g;
  list<string> lp(alloc);
  lp.shm_init();

  for (int i = 0; i < 30; ++i) {
    lp.emplace_back(std::to_string(i));
  }
  REQUIRE(lp.size() == 30);

  {
    int fcur = 0;
    for (auto num : lp) {
      REQUIRE(num == std::to_string(fcur));
      ++fcur;
    }
  }

  lp.emplace_front("100");
  REQUIRE(lp.front() == "100");
  REQUIRE(lp.size() == 31);

  lp.erase(lp.begin(), lp.begin() + 1);

  {
    int fcur = 0;
    for (auto num : lp) {
      REQUIRE(num == std::to_string(fcur));
      ++fcur;
    }
  }

  lp.erase(lp.begin(), lp.end());
  REQUIRE(lp.size() == 0);

  lp.shm_destroy();
}

TEST_CASE("ListOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("ListOfStrings") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListOfStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
