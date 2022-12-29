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

/**
 * MACROS used to simplify the vector namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME ShmStruct
#define TYPED_CLASS T

/** The base archive used by all ShmStruct objects */
template<typename T>
struct ShmStructArchive : public ShmContainerArchive {
  T obj_;
  template<typename ...Args>
  explicit ShmStructArchive(Args &&...args)
    : obj_(std::forward<Args>(args)...) {}
};
#define DEFINE_SHM_STRUCT_ARCHIVE(TYPE) \
  template<>\
  struct ShmArchive<TYPE> : public ShmStructArchive<TYPE> {\
    template<typename ...Args>\
    ShmArchive(Args&& ...args)\
    : ShmStructArchive<TYPE>(std::forward<Args>(args)...) {}\
  };

/**
 * Used for storing a simple type (int, double, C-style struct, etc) in shared
 * memory using a smart pointer (e.g., manual_ptr, unique_ptr, and shared_ptr).
 * This class is not intended to be used directly, unless you're building a
 * class which inherits from ShmSmartPtr.
 * */
template<typename T>
struct ShmStruct : public ShmContainer<TYPED_CLASS> {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE

  /** Default constructor */
  ShmStruct() = default;

  /** Construct pointer in-place (find allocator) */
  template<typename ...Args>
  void shm_init(Args &&...args) {
    shm_init(reinterpret_cast<ShmArchive<T>*>(NULL),
             reinterpret_cast<Allocator*>(NULL),
             std::forward<Args>(args)...);
  }

  /** Construct pointer in-place (find allocator) */
  template<typename ...Args>
  void shm_init(Allocator *alloc, Args &&...args) {
    shm_init(reinterpret_cast<ShmArchive<T>*>(NULL),
             alloc,
             std::forward<Args>(args)...);
  }

  /**
   * Constructs and stores a simple C type in shared-memory. E.g., a struct
   * or union. Complex structures should look at ShmContainer under
   * data_structures/data_structure.h
   * */
  template<typename ...Args>
  void shm_init(ShmArchive<T> *ar, Allocator *alloc, Args &&...args) {
    ShmContainer<TYPED_CLASS>::shm_init(ar, alloc,
                                        std::forward<Args>(args)...);
  }

  /** Destroy the contents of the ShmStruct */
  void shm_destroy() {
    if (IsNull()) { return; }
    alloc_->Free(header_ptr_);
    SetNull();
  }

  /** Serialize into shared memory */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    ShmContainer<TYPED_CLASS>::shm_serialize(ar);
  }

  /** Deserialize from shared memory */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    ShmContainer<TYPED_CLASS>::shm_deserialize(ar);
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
  const T* get_const() const {
    return &header_->obj_;
  }

  /** Convert into a const reference */
  const T& get_ref_const() const {
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

/** Define archives for basic integer types */
DEFINE_SHM_STRUCT_ARCHIVE(int8_t)
DEFINE_SHM_STRUCT_ARCHIVE(int16_t)
DEFINE_SHM_STRUCT_ARCHIVE(int32_t)
DEFINE_SHM_STRUCT_ARCHIVE(int64_t)
DEFINE_SHM_STRUCT_ARCHIVE(uint8_t)
DEFINE_SHM_STRUCT_ARCHIVE(uint16_t)
DEFINE_SHM_STRUCT_ARCHIVE(uint32_t)
DEFINE_SHM_STRUCT_ARCHIVE(uint64_t)
DEFINE_SHM_STRUCT_ARCHIVE(float)
DEFINE_SHM_STRUCT_ARCHIVE(double)
DEFINE_SHM_STRUCT_ARCHIVE(long double)

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_DATA_STRUCTURES_SHM_POINTER_H_
