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

#include <string>
#include <labstor/data_structures/string.h>

template<typename T>
class AllocatorTestSuite {
 public:
  std::string alloc_type_;
  Allocator *alloc_;

  AllocatorTestSuite(Allocator *alloc) {
    if constexpr(std::is_same_v<lipc::StackAllocator, T>) {
      alloc_type_ = "lipc::StackAllocator";
    } else if constexpr(std::is_same_v<lipc::PageAllocator, T>) {
      alloc_type_ = "lipc::PageAllocator";
    }
  }

  void TestOutput(const std::string &test_name, Timer &t) {
    printf("%s, %s, %lf\n",
           test_name.c_str(),
           alloc_type_.c_str(),
           t.GetMsec());
  }

  void AllocateSameSize(int count, size_t size) {
    Timer t;
    t.Resume();
    for (int i = 0; i < count; ++i) {
      Pointer p = alloc_->Allocate(size);
      alloc_->Free(p);
    }
    t.Pause();

    TestOutput("ConstructDestructTest", t);
  }
};

template<typename T>
void AllocatorTest() {
  allocator_id_t alloc_id(0, 1);
  Allocator *alloc = LABSTOR_MEMORY_MANAGER->CreateAllocator(
    AllocatorType::kStackAllocator, shm_url, alloc_id, 0);
  AllocatorTestSuite<lipc::StackAllocator>(alloc).AllocateSameSize(100000, 10);
  AllocatorTestSuite<T>(alloc).AllocateSameSize(100000, 10);
}

TEST_CASE("AllocatorBenchmark") {
  AllocatorTest<lipc::StackAllocator>();
}