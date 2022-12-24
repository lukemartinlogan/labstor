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


#ifndef LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_
#define LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include "labstor/data_structures/internal/shm_macros.h"
#include <labstor/constants/data_structure_singleton_macros.h>

#include "labstor/data_structures/internal/shm_archive.h"
#include "labstor/data_structures/internal/shm_struct.h"
#include "labstor/data_structures/internal/shm_construct.h"

namespace labstor::ipc {

/**
 * Indicates a data structure represents a memory paradigm for Shm.
 * */
class ShmSmartPointer : public ShmArchiveable {};

/**
 * A base class used for creating shared-memory pointer management
 * classes (manual_ptr, unique_ptr, shared_ptr).
 *
 * Smart pointers are not stored directly in shared memory. They are
 * wrappers around shared-memory objects (ShmContainer or ShmStruct)
 * which are used for constructing destructing these objects.
 * */
template<typename T>
class ShmSmartPtr : public ShmSmartPointer {
 public:
  typedef SHM_T_OR_SHM_PTR_T(T) T_Ptr;
  T_Ptr obj_;  /**< T for archiveable objects, ShmStruct<T> for C-style */

 public:
  /** Sets this pointer to NULL */
  void SetNull() {
    obj_.SetNull();
  }

  /** Checks if this pointer is null */
  bool IsNull() {
    return obj_.IsNull();
  }

  /** Gets a pointer to the internal object */
  T* get() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return &obj_;
    } else {
      return obj_.get();
    }
  }

  /** Gets a pointer to the internal object */
  const T* get_const() const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return &obj_;
    } else {
      return obj_.get_const();
    }
  }

  /** Gets a reference to the internal object */
  T& get_ref() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return obj_;
    } else {
      return obj_.get_ref();
    }
  }

  /** Gets a reference to the internal object */
  const T& get_ref_const() const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return obj_;
    } else {
      return obj_.get_ref_const();
    }
  }

  /** Gets a reference to the internal object using * */
  T& operator*() {
    return get_ref();
  }

  /** Gets a pointer to the internal object */
  T* operator->() {
    return get();
  }

  /** Destroy the data allocated by this pointer */
  void shm_destroy() {
    obj_.shm_destroy();
  }

  /** Serialize into a raw pointer */
  void shm_serialize(Pointer &header_ptr) const {
    obj_.shm_serialize(header_ptr);
  }

  /** Deserialize from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr) {
    obj_.shm_deserialize(header_ptr);
  }
};



}  // namespace labstor::ipc

/**
 * Namespace simplification for a SHM data structure pointer
 * */
#define SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T) \
  using ShmSmartPtr<T>::obj_;\
  using ShmSmartPtr<T>::get;\
  using ShmSmartPtr<T>::get_ref;\
  using ShmSmartPtr<T>::get_const;\
  using ShmSmartPtr<T>::get_ref_const;\
  using ShmSmartPtr<T>::SetNull;\
  using ShmSmartPtr<T>::IsNull;\
  using ShmSmartPtr<T>::shm_destroy;\
  using ShmSmartPtr<T>::shm_serialize;\
  using ShmSmartPtr<T>::shm_deserialize;

#endif  // LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_
