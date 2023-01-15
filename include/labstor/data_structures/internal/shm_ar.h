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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_AR_H_
#define LABSTOR_DATA_STRUCTURES_SHM_AR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "shm_construct.h"

namespace labstor::ipc {

/**
 * Constructs a ShmArchive in-place
 * */
template<typename T>
class _shm_ar_shm {
 public:
  typedef typename T::header_t header_t;
  header_t obj_hdr_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_shm(Allocator *alloc, Args&& ...args) {
    ShmArchive<T> ar = internal_ref(alloc);
    make_shm_ar<T>(ar, alloc, std::forward<Args>(args)...);
  }

  /** Destructor */
  inline ~_shm_ar_shm() = default;

  /** Shm destructor */
  inline void shm_destroy(Allocator *alloc) {
    auto ar = internal_ref(alloc);
    T(ar).shm_destroy();
  }

  /** Returns a reference to the internal object */
  inline ShmArchive<T> internal_ref(Allocator *alloc) {
    return ShmArchive<T>(alloc->Convert<header_t, Pointer>(&obj_hdr_));
  }

  /** Returns a reference to the internal object */
  inline ShmArchive<T> internal_ref(Allocator *alloc) const {
    return ShmArchive<T>(alloc->Convert<header_t, Pointer>(&obj_hdr_));
  }

  /** Move constructor */
  inline _shm_ar_shm(_shm_ar_shm &&other) noexcept {}

  /** Copy constructor */
  inline _shm_ar_shm(const _shm_ar_shm &other) = delete;
};

/**
 * Constructs an object in-place
 * */
template<typename T>
class _shm_ar_noshm {
 public:
  T obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_noshm(Allocator *alloc, Args&& ...args)
  : obj_(std::forward<Args>(args)...) {
    (void) alloc;
  }

  /** Shm destructor */
  inline void shm_destroy(Allocator *alloc) {}

  /** Destructor. Does nothing. */
  inline  ~_shm_ar_noshm() = default;

  /** Returns a reference to the internal object */
  inline T& internal_ref(Allocator *alloc) {
    (void) alloc;
    return obj_;
  }

  /** Returns a reference to the internal object */
  inline T& internal_ref(Allocator *alloc) const {
    (void) alloc;
    return obj_;
  }

  /** Move constructor */
  inline _shm_ar_noshm(_shm_ar_noshm &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  inline _shm_ar_noshm(const _shm_ar_noshm &other) = delete;
};

/**
 * Whether or not to use _shm_ar_shm or _shm_ar_noshm
 * */
#define SHM_MAKE_T_OR_ARCHIVE(T) \
  SHM_X_OR_Y(T, _shm_ar_shm<T>, _shm_ar_noshm<T>)

/**
 * Used for data structures which intend to store:
 * 1. An archive if the data type is SHM_SERIALIZEABLE
 * 2. The raw type if the data type is anything else
 *
 * E.g., used in unordered_map for storing collision entries.
 * E.g., used in a list for storing list entries.
 * */
template<typename T>
class shm_ar {
 public:
  typedef SHM_ARCHIVE_OR_REF(T) T_Ar;
  SHM_MAKE_T_OR_ARCHIVE(T) obj_;

  /** Construct + store object */
  template<typename ...Args>
  explicit shm_ar(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor */
  inline ~shm_ar() = default;

  /** Returns a reference to the internal object */
  inline T_Ar internal_ref(Allocator *alloc) {
    return obj_.internal_ref(alloc);
  }

  /** Returns a reference to the internal object */
  inline T_Ar internal_ref(Allocator *alloc) const {
    return obj_.internal_ref(alloc);
  }

  /** Shm destructor */
  inline void shm_destroy(Allocator *alloc) {
    obj_.shm_destroy(alloc);
  }

  /** Move constructor */
  inline shm_ar(shm_ar &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  inline shm_ar(const shm_ar &other) = delete;
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_AR_H_
