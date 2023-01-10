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


#include <labstor/memory/allocator/malloc_allocator.h>

namespace labstor::ipc {

void MallocAllocator::shm_init(MemoryBackend *backend,
                               allocator_id_t id,
                               size_t custom_header_size) {
  backend_ = backend;
  header_ = reinterpret_cast<MallocAllocatorHeader*>(backend_->data_);
  custom_header_ = GetCustomHeader<char>();
  header_->Configure(id, custom_header_size);
}

void MallocAllocator::shm_deserialize(MemoryBackend *backend) {
  throw NOT_IMPLEMENTED.format("MallocAllocator::shm_deserialize");
}

size_t MallocAllocator::GetCurrentlyAllocatedSize() {
  throw NOT_IMPLEMENTED.format("MallocAllocator");
}

Pointer MallocAllocator::Allocate(size_t size) {
  return Pointer(GetId(), (size_t)malloc(size));
}

Pointer MallocAllocator::AlignedAllocate(size_t size, size_t alignment) {
  return Pointer(GetId(), (size_t)aligned_alloc(alignment, size));
}

bool MallocAllocator::ReallocateNoNullCheck(Pointer &p, size_t new_size) {
  Pointer old_p = p;
  p = Pointer(GetId(), (size_t)realloc((void*)p.off_.load(), new_size));
  return p.off_ == old_p.off_;
}

void MallocAllocator::FreeNoNullCheck(Pointer &p) {
  free((void*)p.off_.load());
}

}  // namespace labstor::ipc
