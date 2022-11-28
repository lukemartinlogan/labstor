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
#include <labstor/data_structures/lockless/vector.h>
#include <labstor/memory/allocator/page_allocator.h>


using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;

Allocator *alloc_g = nullptr;

void Pretest(AllocatorType type) {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 0);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          shm_url);
  mem_mngr->CreateAllocator(type,
                            shm_url,
                            alloc_id);
  alloc_g = mem_mngr->GetAllocator(alloc_id);
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
  alloc_g = nullptr;
}

TEST_CASE("VectorSimple") {
  Allocator *alloc = alloc_g;
  labstor::ipc::lockless::vector<int> vec(alloc);

  vec.reserve(10);

  for (int i = 0; i < 30; ++i) {
    vec.emplace_back(i);
  }
  REQUIRE(vec.size() == 30);
  for (int i = 0; i < 30; ++i) {
    REQUIRE(vec[i] == i);
  }

  int fcur = 0;
  for (auto num : vec) {
    REQUIRE(num == fcur);
    ++fcur;
  }

  int rcur = (int)vec.size() - 1;
  for (auto num_iter = vec.rbegin(); num_iter != vec.rend(); ++num_iter) {
    REQUIRE((*num_iter) == rcur);
    --rcur;
  }

  vec.emplace(vec.begin(), 100);
  REQUIRE(vec[0] == 100);
  REQUIRE(vec.size() == 31);
  for (int i = 1; i < vec.size(); ++i) {
    REQUIRE(vec[i] == i - 1);
  }

  vec.erase(vec.begin(), vec.begin() + 1);
  REQUIRE(vec.size() == 30);
  for (int i = 0; i < vec.size(); ++i) {
    REQUIRE(vec[i] == i);
  }

  vec.erase(vec.begin(), vec.end());
  REQUIRE(vec.size() == 0);
}
