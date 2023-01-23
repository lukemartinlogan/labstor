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

namespace labstor::ipc {

/**
 * Constructs a TypedPointer in-place
 * */
template<typename T>
class _ShmHeaderOrT_Header {
 public:
  typedef typename T::header_t header_t;
  header_t obj_hdr_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _ShmHeaderOrT_Header(Allocator *alloc, Args&& ...args) {
    T(obj_hdr_, alloc, std::forward<Args>(args)...).UnsetDestructable();
  }

  /** Destructor */
  ~_ShmHeaderOrT_Header() = default;

  /** Shm destructor */
  void shm_destroy(Allocator *alloc) {
    auto ar = internal_ref(alloc);
    T obj;
    obj.shm_deserialize(ar);
    obj.shm_destroy();
  }

  /** Returns a reference to the internal object */
  TypedPointer<T> internal_ref(Allocator *alloc) {
    return TypedPointer<T>(alloc->Convert<header_t, Pointer>(&obj_hdr_));
  }

  /** Returns a reference to the internal object */
  TypedPointer<T> internal_ref(Allocator *alloc) const {
    return TypedPointer<T>(alloc->Convert<header_t, Pointer>(&obj_hdr_));
  }

  /** Default constructor */
  _ShmHeaderOrT_Header() {}

  /** Move constructor */
  _ShmHeaderOrT_Header(_ShmHeaderOrT_Header &&other) noexcept
  : obj_hdr_(std::move(other.obj_hdr_)) {}

  /** Move assignment operator */
  _ShmHeaderOrT_Header& operator=(_ShmHeaderOrT_Header &&other) noexcept {
    obj_hdr_ = std::move(other.obj_hdr_);
    return *this;
  }

  /** Copy constructor */
  _ShmHeaderOrT_Header(const _ShmHeaderOrT_Header &other)
  : obj_hdr_(other.obj_hdr_) {
  }

  /** Copy assignment operator */
  _ShmHeaderOrT_Header& operator=(const _ShmHeaderOrT_Header &other) {
    obj_hdr_ = other.obj_hdr_;
  }
};

/**
 * Constructs an object in-place
 * */
template<typename T>
class _ShmHeaderOrT_T {
 public:
  T obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _ShmHeaderOrT_T(Allocator *alloc, Args&& ...args)
  : obj_(std::forward<Args>(args)...) {
    (void) alloc;
  }

  /** Shm destructor */
  void shm_destroy(Allocator *alloc) {}

  /** Destructor. Does nothing. */
   ~_ShmHeaderOrT_T() = default;

  /** Returns a reference to the internal object */
  T& internal_ref(Allocator *alloc) {
    (void) alloc;
    return obj_;
  }

  /** Returns a reference to the internal object */
  T& internal_ref(Allocator *alloc) const {
    (void) alloc;
    return obj_;
  }

  /** Default constructor */
  _ShmHeaderOrT_T() : obj_() {}

  /** Move constructor */
  _ShmHeaderOrT_T(_ShmHeaderOrT_T &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Move assignment operator */
  _ShmHeaderOrT_T& operator=(_ShmHeaderOrT_T &&other) noexcept {
    obj_ = std::move(other.obj_);
    return *this;
  }

  /** Copy constructor */
  _ShmHeaderOrT_T(const _ShmHeaderOrT_T &other)
  : obj_(other.obj_) {}

  /** Copy assignment operator */
  _ShmHeaderOrT_T& operator=(const _ShmHeaderOrT_T &other) {
    obj_ = other.obj_;
  }
};

/**
 * Whether or not to use _ShmHeaderOrT or _ShmHeaderOrT_T
 * */
#define SHM_MAKE_HEADER_OR_T(T) \
  SHM_X_OR_Y(T, _ShmHeaderOrT_Header<T>, _ShmHeaderOrT_T<T>)

/**
 * Used for data structures which intend to store:
 * 1. An archive if the data type is SHM_ARCHIVEABLE
 * 2. The raw type if the data type is anything else
 *
 * E.g., used in unordered_map for storing collision entries.
 * E.g., used in a list for storing list entries.
 * */
template<typename T>
class ShmHeaderOrT {
 public:
  typedef SHM_ARCHIVE_OR_REF(T) T_Ar;
  SHM_MAKE_HEADER_OR_T(T) obj_;

  /** Construct + store object */
  template<typename ...Args>
  explicit ShmHeaderOrT(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor */
  ~ShmHeaderOrT() = default;

  /** Returns a reference to the internal object */
  T_Ar internal_ref(Allocator *alloc) {
    return obj_.internal_ref(alloc);
  }

  /** Returns a reference to the internal object */
  T_Ar internal_ref(Allocator *alloc) const {
    return obj_.internal_ref(alloc);
  }

  /** Shm destructor */
  void shm_destroy(Allocator *alloc) {
    obj_.shm_destroy(alloc);
  }

  /** Default constructor */
  ShmHeaderOrT() = default;

  /** Move constructor */
  ShmHeaderOrT(ShmHeaderOrT &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Move assignment operator */
  ShmHeaderOrT& operator=(ShmHeaderOrT &&other) noexcept {
    obj_ = std::move(other.obj_);
    return *this;
  }

  /** Copy constructor */
  ShmHeaderOrT(const ShmHeaderOrT &other)
  : obj_(other.obj_) {}

  /** Copy assignment operator */
  ShmHeaderOrT& operator=(const ShmHeaderOrT &other) {
    obj_ = other.obj_;
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_AR_H_
