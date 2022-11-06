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

#include <labstor/memory/backend/posix_shm_mmap.h>

using labstor::memory::MemorySlot;
using labstor::memory::PosixShmMmap;

TEST_CASE("BackendReserve") {
  PosixShmMmap b1("shmem_test");
  b1.Create();

  // Reserve + Map 8GB of memory
  b1.CreateSlot(GIGABYTES(8));
  sleep(2);

  // Set 2GB of SHMEM
  auto &slot = b1.GetSlot(1);
  memset(slot.ptr_, 0, GIGABYTES(2));
  sleep(2);

  // Destroy SHMEM
  b1.Destroy();
}
