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
#include "labstor/util/type_switch.h"

namespace labstor::ipc {

/**
 * A data structure is a struct around shm_ar objects, which
 * are not directly archiveable, but still use the "Allocator" parameter
 * internally to allocate shared objects.
 * */
class ShmArchiveableWrapper {};

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
    : obj_(make_shm_ar<T>(internal_ref(), std::forward<Args>(args)...)) {
  }

  /** Move object here */
  explicit _shm_ar_shm(Allocator *alloc, ShmMove<T> other)
    : obj_(make_shm_ar<T>(internal_ref(), alloc, ShmMove(other.obj_))) {
  }

  /** Move other archive here */
  _shm_ar_shm(Allocator *alloc, ShmMove<_shm_ar_shm> other) noexcept
    : obj_(make_shm_ar<T>(internal_ref(), alloc, ShmMove(other.obj_))) {}

  /** Copy constructor */
  _shm_ar_shm(const _shm_ar_shm &other) = delete;

  /** Destructor. */
  ~_shm_ar_shm() {
    mptr<T>(obj_).shm_destroy();
  }

  /** Returns copy of the deserialized ShmArchive (just a few pointers) */
  T data() {
    return T(obj_);
  }

  /** Returns a reference to the internal object */
  ShmArchive<T>& internal_ref() {
    return obj_;
  }

  /** Returns a pointer to the internal object */
  ShmArchive<T>* internal_ptr() {
    return &obj_;
  }
};

/**
 * Constructs a wrapper around shm_ar in-place
 * */
template<typename T>
class _shm_ar_shm_wrap {
 public:
  T obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_shm_wrap(Args&& ...args)
    : obj_(std::forward<Args>(args)...) {}

  /** Move object here */
  explicit _shm_ar_shm_wrap(Allocator *alloc, ShmMove<T> other)
    : obj_(alloc, other) {
  }

  /** Move other archive here */
  explicit _shm_ar_shm_wrap(Allocator *alloc, ShmMove<_shm_ar_shm_wrap> other) noexcept
    : obj_(alloc, ShmMove(other.obj_)) {}

  /** Copy constructor */
  _shm_ar_shm_wrap(const _shm_ar_shm_wrap &other) = delete;

  /** Destructor. Does nothing. */
  ~_shm_ar_shm_wrap() = default;

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

  /** Construct + store object (ignore alloc param) */
  template<typename ...Args>
  explicit _shm_ar_noshm(Allocator *alloc,
                         Args&& ...args)
    : _shm_ar_noshm(std::forward<Args>(args)...) {
    (void) alloc;
  }

  /** Move object here */
  explicit _shm_ar_noshm(ShmMove<T> other)
    : obj_(std::move(other.obj_)) {}

  /** Move other archive here */
  explicit _shm_ar_noshm(ShmMove<_shm_ar_noshm> other)
    : obj_(std::move(other.obj_)) {}

  /** Move other archive here */
  explicit _shm_ar_noshm(ShmMove<_shm_ar_shm<T>> other)
    : obj_(std::move(other.obj_.obj_)) {}

  /** Copy constructor */
  _shm_ar_noshm(const _shm_ar_noshm &other) = delete;

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
};

/**
 * Whether or not to use _shm_ar_shm or _shm_ar_noshm
 * */
#define SHM_SELECT_AR_TYPE(T)\
  typename static_switch<\
    T,\
    static_case<ShmArchiveable, _shm_ar_shm<T>>,\
    static_case<ShmArchiveableWrapper, _shm_ar_shm_wrap<T>>,\
    static_case<_shm_ar_noshm<T>>\
  >::type

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

  SHM_SELECT_AR_TYPE(T) obj_;

  /** Construct + store object */
  template<typename ...Args>
  explicit shm_ar(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Move constructor */
  explicit shm_ar(Allocator *alloc, ShmMove<shm_ar> other) noexcept
    : obj_(alloc, ShmMove(other.obj_)) {}

  /** Copy constructor */
  shm_ar(const shm_ar &other) = delete;

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
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_AR_H_
