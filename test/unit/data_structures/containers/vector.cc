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
#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/data_structures/thread_unsafe/list.h"
#include "labstor/data_structures/string.h"

using labstor::ipc::vector;
using labstor::ipc::list;
using labstor::ipc::string;

void VectorOfIntTest() {
  Allocator *alloc = alloc_g;
  labstor::ipc::vector<int> vec(alloc);

  // Reserve 10 slots in shared memory
  {
    vec.reserve(10);
    REQUIRE(vec.size() == 0);
  }

  // Emplace 30 elements into the vector (forces growth)
  {
    for (int i = 0; i < 30; ++i) {
      vec.emplace_back(i);
    }
    REQUIRE(vec.size() == 30);
    for (int i = 0; i < 30; ++i) {
      REQUIRE(vec[i] == i);
    }
  }

  // Forward iterator test
  {
    int fcur = 0;
    for (auto num : vec) {
      REQUIRE(num == fcur);
      ++fcur;
    }
    REQUIRE(fcur == vec.size());
  }

  // Reverse iterator test
  {
    int rcur = (int) vec.size() - 1;
    for (auto num_iter = vec.rbegin(); num_iter != vec.rend(); ++num_iter) {
      REQUIRE((*num_iter) == rcur);
      --rcur;
    }
    REQUIRE(rcur == -1);
  }

  // Emplace at front of vector
  {
    vec.emplace(vec.begin(), 100);
    REQUIRE(vec[0] == 100);
    REQUIRE(vec.size() == 31);
    for (int i = 1; i < vec.size(); ++i) {
      REQUIRE(vec[i] == i - 1);
    }
  }

  // Erase first element
  {
    vec.erase(vec.begin(), vec.begin() + 1);
    REQUIRE(vec.size() == 30);
    for (int i = 0; i < vec.size(); ++i) {
      REQUIRE(vec[i] == i);
    }
  }

  // Copy vector (constructor)
  {
    labstor::ipc::vector<int> cpy(vec);
    REQUIRE(cpy.size() == 30);
    for (int i = 0; i < cpy.size(); ++i) {
      REQUIRE(cpy[i] == i);
    }
  }

  // Copy vector (assign)
  {
    labstor::ipc::vector<int> cpy;
    cpy = vec;
    REQUIRE(cpy.size() == 30);
    for (int i = 0; i < cpy.size(); ++i) {
      REQUIRE(cpy[i] == i);
    }
  }

  // move vector
  {
    labstor::ipc::vector<int> cpy(std::move(vec));
    REQUIRE(cpy.size() == 30);
    for (int i = 0; i < cpy.size(); ++i) {
      REQUIRE(cpy[i] == i);
    }

    vec = std::move(cpy);
    REQUIRE(vec.size() == 30);
    for (int i = 0; i < vec.size(); ++i) {
      REQUIRE(vec[i] == i);
    }
  }

  // Copy vector (copy constructor)
  {
    std::vector<int> orig;
    for (int i = 0; i < 30; ++i) {
      orig.emplace_back(i);
    }
    labstor::ipc::vector<int> cpy(nullptr, orig);
    REQUIRE(cpy.size() == 30);
    for (int i = 0; i < cpy.size(); ++i) {
      REQUIRE(cpy[i] == i);
    }
  }

  // Erase entire vector
  {
    vec.erase(vec.begin(), vec.end());
    REQUIRE(vec.size() == 0);
  }
}

void VectorOfStringTest() {
  Allocator *alloc = alloc_g;
  vector<string> vec(alloc);
  int max_count = 5;

  // Reserve 10 slots in shared memory
  {
    vec.reserve(10);
    REQUIRE(vec.size() == 0);
  }


  // Emplace 30 elements into the vector (forces growth)
  {
    for (int i = 0; i < max_count; ++i) {
      vec.emplace_back(std::to_string(i));
    }
    REQUIRE(vec.size() == max_count);
    for (int i = 0; i < max_count; ++i) {
      vec[i];
      // REQUIRE(vec[i] == std::to_string(i));
    }
  }

  // Forward iterator test
  {
    vec.emplace(vec.begin(), "100");
    REQUIRE(vec[0] == "100");
    REQUIRE(vec.size() == max_count + 1);
    for (int i = 1; i < vec.size(); ++i) {
      REQUIRE(vec[i] == std::to_string(i - 1));
    }
  }

  // Reverse iterator test
  {
    vec.erase(vec.begin(), vec.begin() + 1);
    REQUIRE(vec.size() == max_count);
    for (int i = 0; i < vec.size(); ++i) {
      REQUIRE(vec[i] == std::to_string(i));
    }
  }

  // Modify the fourth list entry (move assignment)
  {
    auto iter = vec.begin() + 4;
    (~iter) = std::move(string(alloc, "25"));
  }

  // Verify the modification took place
  {
    auto iter = vec.begin() + 4;
    REQUIRE((*iter) == "25");
  }

  // Modify the fourth list entry (copy assignment)
  {
    auto iter = vec.begin() + 4;
    string text(alloc, "50");
    (~iter) = text;
  }

  // Verify the modification took place
  {
    auto iter = vec.begin() + 4;
    REQUIRE((*iter) == "50");
  }

  // Erase entire vector
  {
    vec.erase(vec.begin(), vec.end());
    REQUIRE(vec.size() == 0);
  }
}

void VectorOfListOfStringTest() {
  Allocator *alloc = alloc_g;
  vector<list<string>> vec(alloc);

  vec.resize(10);
  for (auto bkt : vec) {
    bkt.emplace_back("hello");
  }
  vec.clear();
}

TEST_CASE("VectorOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  VectorOfIntTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("VectorOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  //VectorOfStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("VectorOfListOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  //VectorOfListOfStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
