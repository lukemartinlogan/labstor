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


/**
 * Create a data structure in either shared or regular memory.
 * Destroy the data structure when the unique pointer is deconstructed
 * */

#ifndef LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
#define LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

/**
 * MACROS to simplify the unique_ptr namespace
 * */
#define CLASS_NAME unique_ptr
#define TYPED_CLASS unique_ptr<T>

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class unique_ptr : public ShmDataStructurePointer<T> {
 public:
  SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T);

 public:
  /** Default constructor */
  unique_ptr() = default;

  /** Destroys all allocated memory */
  ~unique_ptr() {
    shm_destroy();
  }

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    obj_.shm_init(std::forward<Args>(args)...);
  }

  /** Disable the copy constructor */
  unique_ptr(const unique_ptr &other) = delete;

  /** Move constructor */
  unique_ptr(unique_ptr&& source) noexcept {
    if (this != &source) {
      obj_.WeakMove(source.obj_);
    }
  }

  /** Serialize into a ShmArchive<unique_ptr> */
  SHM_SERIALIZE_WRAPPER(unique_ptr)

  /** Disables the assignment operator */
  void operator=(unique_ptr<T> &&other) = delete;
};

template<typename T>
using uptr = unique_ptr<T>;

template<typename T, typename ...Args>
static uptr<T> make_uptr(Args&& ...args) {
  uptr<T> ptr;
  ptr.shm_init(std::forward<Args>(args)...);
  return ptr;
}

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

namespace std {

/** Hash function for unique_ptr */
template<typename T>
struct hash<labstor::ipc::unique_ptr<T>> {
  size_t operator()(const labstor::ipc::unique_ptr<T> &obj) const {
    return std::hash<T>{}(obj.get_ref_const());
  }
};

}  // namespace std

#endif  // LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
