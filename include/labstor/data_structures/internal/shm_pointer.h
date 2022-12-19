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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_
#define LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_serialize.h"
#include "shm_archive.h"

namespace labstor::ipc {

/**
 * Indicates a data structure represents a memory paradigm for Shm.
 * */
class ShmSmartPointer : public ShmSerializeable {};

/**
 * Used for storing a simple type (C-style struct, etc) in shared
 * memory semantically.
 *
 * Called internally by ShmArchive.
 * Called internally by manual_ptr, unique_ptr, and shared_ptr
 * */
template<typename T>
struct ShmPointer : public ShmSerializer<T> {
  SHM_SERIALIZER_TEMPLATE(T)

  /** Default constructor -- Does nothing */
  ShmPointer() = default;

  /** Construct pointer in-place (find allocator) */
  template<typename ...Args>
  void shm_init(Args &&...args) {
    shm_init(reinterpret_cast<Allocator *>(NULL),
             std::forward<Args>(args)...);
  }

  /**
   * Constructs and stores a simple C type in shared-memory. E.g., a struct
   * or union. Complex structures should look at ShmDataStructure under
   * data_structures/data_structure.h
   * */
  template<typename ...Args>
  void shm_init(Allocator *alloc, Args &&...args) {
    alloc_ = alloc;
    if (alloc_ == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    }
    header_ = alloc_->template
      AllocateConstructObjs<T>(1, header_ptr_, std::forward<Args>(args)...);
  }

  /** Destroy the contents of the ShmPointer */
  void shm_destroy() {
    if (IsNull()) { return; }
    alloc_->Free(header_ptr_);
  }

  /** (De)serialize ShmPointer into ShmArchive<T> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(T)

  /** Convert the pointer to a pointer */
  T *get() {
    return header_;
  }

  /** Convert into a reference */
  T &get_ref() {
    return *get();
  }

  /** Convert the pointer to const pointer */
  T *get_const() const {
    return header_;
  }

  /** Convert into a const reference */
  T &get_ref_const() const {
    return *get_const();
  }

  /** Convert into a pointer */
  T *operator->() {
    return get();
  }

  /** Convert into a reference */
  T &operator*() {
    return get_ref();
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_
