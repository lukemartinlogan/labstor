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
#include "shm_archive.h"
#include "shm_container.h"

namespace labstor::ipc {

/** Create a ShmHeader for the simple type */
template<typename T>
struct ShmSimpleHeader : public lipc::ShmBaseHeader {
  T obj_;

  template<typename ...Args>
  ShmSimpleHeader(Args&& ...args) : obj_(std::forward<Args>(args)...) {}
};

/**
 * MACROS used to simplify the vector namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME ShmStruct
#define TYPED_CLASS T

/**
 * Used for storing a simple type (int, double, C-style struct, etc) in shared
 * memory.
 *
 * Called internally by manual_ptr, unique_ptr, and shared_ptr
 * */
template<typename T>
struct ShmStruct : public ShmContainer<T, ShmSimpleHeader<T>> {
 public:
  SHM_CONTAINER_TEMPLATE_X(CLASS_NAME, TYPED_CLASS, ShmSimpleHeader<T>)

  /** Default constructor. Does nothing. */
  ShmStruct() = default;

  /**
   * Constructs and stores a simple C type in shared-memory. E.g., a struct
   * or union. Complex structures should look at ShmContainer under
   * data_structures/data_structure.h
   * */
  template<typename ...Args>
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, Args &&...args) {
    shm_init_header(ar, alloc, std::forward<Args>(args)...);
  }

  /** Store into shared memory */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize_header(ar.header_ptr_);
  }

  /** Load from shared memory */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize_header(ar.header_ptr_);
  }

  /** Destroy the contents of the ShmStruct */
  void shm_destroy(bool destroy_header = true) {
    SHM_DESTROY_DATA_START
    SHM_DESTROY_DATA_END
    SHM_DESTROY_END
  }

  /** Convert the pointer to a pointer */
  T* get() {
    return &header_->obj_;
  }

  /** Convert into a reference */
  T& get_ref() {
    return *get();
  }

  /** Convert the pointer to const pointer */
  T* get_const() const {
    return &header_->obj_;
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

  /** Move constructor */
  void WeakMove(ShmStruct &other) {
    SHM_WEAK_MOVE_START(SHM_WEAK_MOVE_DEFAULT)
    *header_ = *(other.header_);
    SHM_WEAK_MOVE_END()
  }

  /** Copy constructor */
  void StrongCopy(const ShmStruct &other) {
    SHM_STRONG_COPY_START(SHM_STRONG_COPY_DEFAULT)
    *header_ = *(other.header_);
    SHM_STRONG_COPY_END()
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

#endif  // LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_
