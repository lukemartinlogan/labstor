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
#include "shm_data_structure.h"
#include "shm_archive.h"

namespace labstor::ipc {

/**
 * MACROS used to simplify the vector namespace
 * Used as inputs to the SHM_DATA_STRUCTURE_TEMPLATE
 * */
#define CLASS_NAME ShmSimplePointer
#define TYPED_CLASS T
#define TYPED_HEADER T

/**
 * A wrapper around a process-independent pointer for storing
 * a single complex shared-memory data structure
 * */
template<typename T>
struct ShmPointerArchive {
 public:
  Pointer header_ptr_;

  /** Default constructor */
  ShmPointerArchive() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get() {
    return header_ptr_;
  }

  /** Get the process-independent pointer */
  inline const Pointer& GetConst() {
    return header_ptr_;
  }

  /** Creates a ShmPointerArchive from a header pointer */
  explicit ShmPointerArchive(Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Creates a ShmPointerArchive from a header pointer */
  explicit ShmPointerArchive(const Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Copies a ShmPointerArchive into another */
  ShmPointerArchive(const ShmPointerArchive &other)
    : header_ptr_(other.header_ptr_) {
  }

  /** Moves the data from one archive into another */
  ShmPointerArchive(ShmPointerArchive&& other) noexcept
    : header_ptr_(other.header_ptr_) {
    other.header_ptr_.set_null();
  }

  /** Copies a ShmPointerArchive into another */
  ShmPointerArchive& operator=(const ShmPointerArchive &other) {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
    }
    return *this;
  }

  /** Moves the data from one archive into another */
  ShmPointerArchive& operator=(ShmPointerArchive&& other) noexcept {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
    }
    return *this;
  }
};

/**
 * Used for storing a simple type (C-style struct, etc) in shared
 * memory semantically.
 *
 * Called internally by manual_ptr, unique_ptr, and shared_ptr
 * */
template<typename T>
struct ShmSimplePointer : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 public:
  BASIC_SHM_DATA_STRUCTURE_TEMPLATE

  /** Default constructor */
  ShmSimplePointer() = default;

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
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_init(alloc);
    header_ = alloc_->template
      AllocateConstructObjs<T>(1, header_ptr_, std::forward<Args>(args)...);
  }

  /** Destroy the contents of the ShmSimplePointer */
  void shm_destroy() {
    if (IsNull()) { return; }
    alloc_->Free(header_ptr_);
    SetNull();
  }

  /** Convert the pointer to a pointer */
  T* get() {
    return header_;
  }

  /** Convert into a reference */
  T& get_ref() {
    return *get();
  }

  /** Convert the pointer to const pointer */
  T* get_const() const {
    return header_;
  }

  /** Convert into a const reference */
  T& get_ref_const() const {
    return *get_const();
  }

  /** Convert into a pointer */
  T* operator->() {
    return get();
  }

  /** Convert into a reference */
  T& operator*() {
    return get_ref();
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_
