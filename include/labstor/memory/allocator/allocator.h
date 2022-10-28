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
#include <labstor/memory/backend/memory_backend.h>

namespace labstor::memory {

struct Pointer {
  uint32_t allocator_id_;
  uint32_t slot_id_;
  uint64_t off_;
};

class Allocator : public Attachable {
 private:
  uint64_t id_;
  std::unique_ptr<MemoryBackend> backend_;

 public:
  Allocator(uint64_t id) : id_(id) {}
  virtual Pointer Allocate(size_t size) = 0;
  virtual void Free(Pointer &ptr) = 0;

  void* Convert(Pointer p) {
    auto &slot = backend_->GetSlot(p.slot_id_);
    return slot.ptr_ + p.off_;
  }
};

}  // namespace labstor::memory

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
