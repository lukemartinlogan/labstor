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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
#define LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"

namespace labstor::ipc {

/**
 * Indicates that a data structure can be archived in shared memory
 * and has a corresponding ShmArchive override.
 * */
class ShmArchiveable {
  /**
   * Initialize a SHM data structure in shared-memory.
   * Constructors may wrap around these.
   * */
  // void shm_init(...);

  /**
   * Destroys the shared-memory allocated by the object.
   * Destructors may wrap around this.
   * */
  // void shm_destroy();

  /**
   * Deep copy of an object. Wrapped by copy constructor
   * */
  // void StrongCopy(const CLASS_NAME &other);
  // SHM_INHERIT_COPY_OPS(CLASS_NAME)

  /**
   * Copies only the object's pointers.
   * */
  // void WeakCopy(const CLASS_NAME &other);

  /**
   * Moves the object's contents into another object
   * */
  // void WeakMove(CLASS_NAME &other);
  // SHM_INHERIT_MOVE_OPS(CLASS_NAME)

  /**
   * Store object into a ShmArchive
   * */
  // void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const;
  // SHM_SERIALIZE_OPS(TYPED_CLASS)

  /**
   * Construct object from a ShmArchive.
   * */
  // void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar);
  // SHM_DESERIALIZE_OPS(TYPED_CLASS)
};

/**
 * A wrapper around a process-independent pointer for storing
 * a single complex shared-memory data structure
 * */
template<typename T>
struct ShmArchive {
 public:
  Pointer header_ptr_;

  /** Default constructor */
  ShmArchive() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get() {
    return header_ptr_;
  }

  /** Get the process-independent pointer */
  inline const Pointer& GetConst() {
    return header_ptr_;
  }

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(const Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Copy constructor */
  ShmArchive(const ShmArchive &other)
    : header_ptr_(other.header_ptr_) {
  }

  /** Move constructor */
  ShmArchive(ShmArchive&& other) noexcept
    : header_ptr_(other.header_ptr_) {
    other.header_ptr_.SetNull();
  }

  /** Copy assignmnet operator */
  ShmArchive& operator=(const ShmArchive &other) {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
    }
    return *this;
  }

  /** Move assignment operator. */
  ShmArchive& operator=(ShmArchive&& other) noexcept {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
    }
    return *this;
  }
};

/**
 * Enables a specific ShmArchive type to be serialized
 * */
#define SHM_SERIALIZE_OPS(AR_TYPE)\
  void operator>>(lipc::ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) const {\
    shm_serialize(ar);\
  }

/**
 * Enables a specific ShmArchive type to be deserialized
 * */
#define SHM_DESERIALIZE_OPS(AR_TYPE)\
  void operator<<(                      \
    const lipc::ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) {\
    shm_deserialize(ar);\
  }

/** Enables serialization + deserialization for data structures */
#define SHM_SERIALIZE_DESERIALIZE_OPS(AR_TYPE)\
  SHM_SERIALIZE_OPS(AR_TYPE)\
  SHM_DESERIALIZE_OPS(AR_TYPE)


}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
