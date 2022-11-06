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


#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_

#include <cstdint>
#include <labstor/memory/backend/memory_backend_factory.h>
#include <labstor/memory/memory.h>

namespace labstor::memory {

enum class AllocatorType {
  kPageAllocator,
  kFixedFragmentationAllocator
};

struct AllocatorHeader {
  int allocator_type_;
};

class Allocator {
 protected:
  allocator_id_t id_;
  slot_id_t slot_id_;
  size_t custom_header_size_;
  MemoryBackend *backend_;
  MemorySlot &slot_;

 public:
  explicit Allocator(slot_id_t slot_id, MemoryBackend *backend,
                     size_t custom_header_size) :
    slot_id_(slot_id),
    custom_header_size_(custom_header_size),
    backend_(backend),
    slot_(backend_->GetSlot(slot_id)) {}

  virtual void Create(allocator_id_t id) = 0;
  virtual void Attach() = 0;
  virtual Pointer Allocate(size_t size) = 0;
  virtual void Free(Pointer &ptr) = 0;
  virtual size_t GetInternalHeaderSize() = 0;

  allocator_id_t GetId() {
    return id_;
  }

  slot_id_t GetSlotId() {
    return slot_id_;
  }

  template<typename T>
  T* GetCustomHeader() {
    return reinterpret_cast<T*>(slot_.ptr_ + GetInternalHeaderSize());
  }

  template<typename T>
  T* Convert(Pointer &p) {
    auto &slot = backend_->GetSlot(slot_id_);
    return reinterpret_cast<T*>(slot.ptr_ + p.off_);
  }
};

}  // namespace labstor::memory

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
