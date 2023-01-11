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

class AllocatorTestSuite {
 public:
  std::string alloc_type_;
  Allocator *alloc_;

  AllocatorTestSuite(AllocatorType alloc_type, Allocator *alloc)
    : alloc_(alloc) {
    switch (alloc_type) {
      case AllocatorType::kStackAllocator: {
        alloc_type_ = "lipc::StackAllocator";
        break;
      }
      case AllocatorType::kPageAllocator: {
        alloc_type_ = "lipc::PageAllocator";
        break;
      }
      case AllocatorType::kMallocAllocator: {
        alloc_type_ = "lipc::MallocAllocator";
        break;
      }
    }
  }

  void TestOutput(const std::string &test_name, size_t obj_size, Timer &t) {
    printf("%s, %s, %lu, %lf\n",
           test_name.c_str(),
           alloc_type_.c_str(),
           obj_size,
           t.GetMsec());
  }

  void AllocateAndFreeFixedSize(int count, size_t size) {
    Timer t;
    t.Resume();
    for (int i = 0; i < count; ++i) {
      Pointer p = alloc_->Allocate(size);
      alloc_->Free(p);
    }
    t.Pause();

    TestOutput("ConstructDestructTest", size, t);
  }

  void AllocateThenFreeFixedSize(int count, size_t size) {
    Timer t;
    std::vector<Pointer> cache(count);
    t.Resume();
    for (int i = 0; i < count; ++i) {
      cache[i] = alloc_->Allocate(size);
    }
    for (int i = 0; i < count; ++i) {
      alloc_->Free(cache[i]);
    }
    t.Pause();

    TestOutput("ConstructDestructTest", size, t);
  }
};

template<typename AllocT, typename ...Args>
Allocator* Pretest(MemoryBackendType backend_type,
                   Args&& ...args) {
  static int minor = 1;
  minor += 1;
  allocator_id_t alloc_id(0, minor);
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(backend_type,
                          MemoryManager::kDefaultBackendSize,
                          shm_url);
  return mem_mngr->CreateAllocator<AllocT>(
    shm_url, alloc_id, 0, std::forward<Args>(args)...);
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
}

template<typename AllocT, typename ...Args>
void AllocatorTest(AllocatorType alloc_type,
                   MemoryBackendType backend_type,
                   Args&& ...args) {
  Allocator *alloc = Pretest<AllocT>(
    backend_type, std::forward<Args>(args)...);
  AllocatorTestSuite(alloc_type, alloc).AllocateThenFreeFixedSize(
    (2<<10), MEGABYTES(1));
  Posttest();
}

TEST_CASE("AllocatorBenchmark") {
  /*AllocatorTest(AllocatorType::kStackAllocator,
                MemoryBackendType::kPosixShmMmap);
  AllocatorTest(AllocatorType::kMallocAllocator,
                MemoryBackendType::kNullBackend);*/
  AllocatorTest<lipc::PageAllocator>(
    AllocatorType::kPageAllocator,
    MemoryBackendType::kPosixShmMmap,
    MEGABYTES(1));
}