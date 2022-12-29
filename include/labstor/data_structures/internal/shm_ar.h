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
#include "labstor/data_structures/smart_ptr/manual_ptr.h"

namespace labstor::ipc {

/**
 * Constructs a ShmArchive in-place
 * */
template<typename T>
class _shm_ar_shm : public ShmSmartPointer {
 public:
  ShmArchive<T> obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_shm(Args&& ...args)
    : obj_(make_shm_ar<T>(std::forward<Args>(args)...)) {
  }

  /** Destructor. */
  ~_shm_ar_shm() {
    mptr<T>(obj_).shm_destroy();
  }

  /** Returns copy of the deserialized ShmArchive (just a few pointers) */
  T data() {
    return mptr<T>(obj_).get_ref();
  }

  /** Returns a reference to the internal object */
  ShmArchive<T>& internal_ref() {
    return obj_;
  }

  /** Returns a pointer to the internal object */
  ShmArchive<T>* internal_ptr() {
    return &obj_;
  }

  /** Move constructor */
  _shm_ar_shm(_shm_ar_shm &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  _shm_ar_shm(const _shm_ar_shm &other) = delete;
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
  explicit _shm_ar_noshm(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_noshm(T *ar,
                         Allocator *alloc,
                         Args&& ...args)
    : obj_(std::forward<Args>(args)...) {
    (void) ar;
  }

  /** Destructor. Does nothing. */
  ~_shm_ar_noshm() = default;

  /** Gets the object */
  T& data() {
    return obj_;
  }

  /** Returns a reference to the internal object */
  T& internal_ref() {
    return obj_;
  }

  /** Returns a pointer to the internal object */
  T* internal_ptr() {
    return &obj_;
  }

  /** Move constructor */
  _shm_ar_noshm(_shm_ar_noshm &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  _shm_ar_noshm(const _shm_ar_noshm &other) = delete;
};

/**
 * Whether or not to use _shm_ar_shm or _shm_ar_noshm
 * */
#define SHM_MAKE_T_OR_ARCHIVE(T) \
  SHM_X_OR_Y(T, _shm_ar_shm<T>, _shm_ar_noshm<T>)

#define SHM_ARCHIVE_OR_T(T)\
  SHM_X_OR_Y(T, ShmArchive<T>, T)

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
  typedef SHM_ARCHIVE_OR_T(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

  SHM_MAKE_T_OR_ARCHIVE(T) obj_;

  /** Construct + store object */
  template<typename ...Args>
  explicit shm_ar(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor */
  ~shm_ar() = default;

  /** Returns the data represented by the archive */
  T_Ref data() {
    return obj_.data();
  }

  /** Returns a reference to the internal object */
  T_Ar& internal_ref() {
    return obj_.internal_ref();
  }

  /** Returns a pointer to the internal object */
  T_Ar* internal_ptr() {
    return obj_.internal_ptr();
  }

  /** Move constructor */
  shm_ar(shm_ar &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  shm_ar(const shm_ar &other) = delete;
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_AR_H_
