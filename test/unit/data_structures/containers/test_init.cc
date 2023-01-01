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
#include <mpi.h>

#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/memory/allocator/page_allocator.h"

Allocator *alloc_g = nullptr;

void Pretest(AllocatorType type) {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 1);
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->CreateBackend(MemoryBackendType::kPosixShmMmap,
                          MemoryManager::kDefaultBackendSize,
                          shm_url);
  mem_mngr->CreateAllocator(type, shm_url, alloc_id, 0);
  alloc_g = mem_mngr->GetAllocator(alloc_id);
}

void Posttest() {
  std::string shm_url = "test_allocators";
  auto mem_mngr = LABSTOR_MEMORY_MANAGER;
  mem_mngr->DestroyBackend(shm_url);
  alloc_g = nullptr;
}

void MainPretest() {
  Pretest(AllocatorType::kStackAllocator);
}

void MainPosttest() {
  Posttest();
}
