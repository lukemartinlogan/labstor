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
#include <labstor/data_structures/lockless/string.h>
#include <labstor/memory/allocator/page_allocator.h>

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;
using labstor::ipc::lockless::string;

TEST_CASE("String") {
  Allocator *alloc = alloc_g;

  REQUIRE(std::is_base_of<labstor::ipc::ShmDataStructure<string>, string>::value);
  REQUIRE(IS_SHM_SERIALIZEABLE(string));
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);

  auto text1 = string("hello1");
  REQUIRE(text1 == "hello1");
  REQUIRE(text1 != "h");
  REQUIRE(text1 != "asdfklaf");

  auto text2 = string("hello2");
  REQUIRE(text2 == "hello2");

  auto text3 = text1 + text2;
  REQUIRE(text3 == "hello1hello2");

  text1.shm_destroy();
  text2.shm_destroy();
  text3.shm_destroy();

  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}