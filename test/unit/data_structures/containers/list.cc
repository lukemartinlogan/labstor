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
#include "labstor/data_structures/thread_unsafe/list.h"
#include "labstor/data_structures/string.h"
#include "labstor/memory/allocator/page_allocator.h"

using labstor::ipc::list;
using labstor::ipc::string;

template<typename T>
void ListTest() {
  Allocator *alloc = alloc_g;
  list<T> lp(alloc);

  // Emplace 30 elements
  for (int i = 0; i < 30; ++i) {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, var, i);
    lp.emplace_back(var);
  }
  REQUIRE(lp.size() == 30);

  // Check full iterator
  {
    int fcur = 0;
    for (auto num : lp) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // Copy list (constructor)
  {
    list<T> cpy(lp);
    int fcur = 0;
    REQUIRE(lp.size() == 30);
    REQUIRE(cpy.size() == 30);
    fcur = 0;
    for (auto num : lp) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
    fcur = 0;
    for (auto num : cpy) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // Copy list (assign)
  {
    list<T> cpy;
    cpy = lp;
    int fcur = 0;
    REQUIRE(lp.size() == 30);
    REQUIRE(cpy.size() == 30);
    fcur = 0;
    for (auto num : lp) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
    fcur = 0;
    for (auto num : cpy) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // move vector
  {
    list<T> cpy(std::move(lp));
    int fcur = 0;
    lp = std::move(cpy);
    fcur = 0;
    REQUIRE(lp.size() == 30);
    REQUIRE(cpy.size() == 0);
    for (auto num : lp) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // emplace_front and erase front
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, i0, 100);
    lp.emplace_front(i0);
    REQUIRE(*lp.front() == i0);
    REQUIRE(lp.size() == 31);
    lp.erase(lp.begin(), lp.begin() + 1);
  }

  // Verify the list is still the original list
  {
    int fcur = 0;
    for (auto num : lp) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // Modify the fourth list entry
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, i4, 25);
    auto iter = lp.begin() + 4;
    (**iter) = i4;
  }

  // Verify the modification took place
  {
    CREATE_SET_VAR_TO_INT_OR_STRING(T, i4, 25);
    auto iter = lp.begin() + 4;
    REQUIRE((**iter) == i4);
  }

  // Copy list (copy constructor)
  {
    std::list<T> orig;
    for (int i = 0; i < 30; ++i) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, var, i);
      orig.emplace_back(var);
    }
    labstor::ipc::list<T> cpy(orig);
    REQUIRE(cpy.size() == 30);
    int fcur = 0;
    for (auto num : cpy) {
      CREATE_SET_VAR_TO_INT_OR_STRING(T, fcur_conv, fcur);
      REQUIRE(*num == fcur_conv);
      ++fcur;
    }
  }

  // Erase the entire list
  {
    lp.clear();
    REQUIRE(lp.size() == 0);
  }
}

TEST_CASE("ListOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest<int>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("ListOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest<lipc::string>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
