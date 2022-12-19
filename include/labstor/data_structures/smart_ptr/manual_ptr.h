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


#ifndef LABSTOR_DATA_STRUCTURES_PTR_H_
#define LABSTOR_DATA_STRUCTURES_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "unique_ptr.h"

namespace labstor::ipc {

/**
 * MACROS to simplify the ptr namespace
 * */
#define CLASS_NAME manual_ptr
#define TYPED_CLASS manual_ptr<T>

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class manual_ptr : public ShmSmartPtr<T> {
 public:
  SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T);

 public:
  /** Default constructor does nothing */
  manual_ptr() = default;

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    obj_.shm_init(std::forward<Args>(args)...);
  }

  /** Destructor. Does not free data. */
  ~manual_ptr() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Copy constructor */
  manual_ptr(const manual_ptr &other) {
    WeakCopy(other);
  }

  /** Move constructor */
  manual_ptr(manual_ptr&& source) noexcept {
    WeakMove(source);
  }

  /** Copy assignment operator */
  manual_ptr<T>& operator=(const manual_ptr<T> &other) {
    if (this != &other) {
      WeakCopy(other);
    }
    return *this;
  }

  /** Move assignment operator */
  void WeakMove(manual_ptr &other) {
    obj_.WeakMove(other.obj_);
  }

  /** Copy a manual_ptr */
  void WeakCopy(const manual_ptr &other) {
    obj_.WeakCopy(other.obj_);
  }

  /** Constructor. From a ShmArchive<T> */
  template<typename ...Args>
  explicit manual_ptr(ShmArchive<T> &ar) {
    shm_deserialize(ar);
  }

  /** Constructor. From a ShmArchive<mptr> */
  explicit manual_ptr(ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize(ar);
  }

  /** Constructor. From a ShmArchive<uptr> */
  explicit manual_ptr(ShmArchive<uptr<T>> &ar) {
    shm_deserialize(ar);
  }

  /** (De)serialize the obj from a ShmArchive<T> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(T);

  /** (De)serialize the obj from a ShmArchive<mptr<T>> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(manual_ptr<T>);

  /** Deserialize the obj from a ShmArchive<uptr<T>> */
  SHM_DESERIALIZE_WRAPPER(uptr<T>);
};

template<typename T>
using mptr = manual_ptr<T>;

template<typename T, typename ...Args>
static mptr<T> make_mptr(Args&& ...args) {
  mptr<T> ptr;
  ptr.shm_init(std::forward<Args>(args)...);
  return ptr;
}

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

namespace std {

/** Hash function for ptr */
template<typename T>
struct hash<labstor::ipc::manual_ptr<T>> {
  size_t operator()(const labstor::ipc::manual_ptr<T> &obj) const {
    return std::hash<T>{}(obj.get_ref_const());
  }
};

}  // namespace std

#endif  // LABSTOR_DATA_STRUCTURES_PTR_H_
