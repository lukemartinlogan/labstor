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


#include <labstor/memory/allocator/stack_allocator.h>
#include <labstor/memory/allocator/mp_page.h>

namespace labstor::ipc {

void StackAllocator::shm_init(MemoryBackend *backend,
                             allocator_id_t id,
                             size_t custom_header_size) {
  backend_ = backend;
  header_ = reinterpret_cast<StackAllocatorHeader*>(backend_->data_);
  custom_header_ = reinterpret_cast<char*>(header_ + 1);
  size_t region_off = (custom_header_ - backend_->data_) + custom_header_size;
  size_t region_size = backend_->data_size_ - region_off;
  header_->Configure(id, custom_header_size, region_off, region_size);
}

void StackAllocator::shm_deserialize(MemoryBackend *backend) {
  backend_ = backend;
  header_ = reinterpret_cast<StackAllocatorHeader*>(backend_->data_);
  custom_header_ = reinterpret_cast<char*>(header_ + 1);
}

size_t StackAllocator::GetCurrentlyAllocatedSize() {
  return header_->total_alloc_;
}

Pointer StackAllocator::Allocate(size_t size) {
  size += sizeof(MpPage);
  size_t off = header_->region_off_.fetch_add(size);
  Pointer p(GetId(), off);
  auto hdr = Convert<MpPage>(p);
  hdr->SetAllocated();
  hdr->page_size_ = size;
  header_->region_size_.fetch_sub(hdr->page_size_);
  header_->total_alloc_.fetch_add(hdr->page_size_);
  return p + sizeof(MpPage);
}

Pointer StackAllocator::AlignedAllocate(size_t size, size_t alignment) {
  throw ALIGNED_ALLOC_NOT_SUPPORTED.format();
}

bool StackAllocator::ReallocateNoNullCheck(Pointer &p, size_t new_size) {
  Pointer new_p;
  void *src = Convert<void>(p);
  auto hdr = Convert<MpPage>(p - sizeof(MpPage));
  void *dst = AllocatePtr<void>(new_size, new_p);
  memcpy(dst, src, hdr->page_size_);
  Free(p);
  p = new_p;
  return true;
}

void StackAllocator::FreeNoNullCheck(Pointer &p) {
  auto hdr = Convert<MpPage>(p - sizeof(MpPage));
  if (!hdr->IsAllocated()) {
    throw DOUBLE_FREE;
  }
  hdr->UnsetAllocated();
  header_->total_alloc_.fetch_sub(hdr->page_size_);
}

}  // namespace labstor::ipc
