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
struct ShmArchive;

/** Generates the code for constructors */
#define SHM_INHERIT_CONSTRUCTOR(CLASS_NAME, TYPED_CLASS)\
  template<typename ...Args>\
  explicit CLASS_NAME(Allocator *alloc, Args&& ...args) {\
    shm_init(nullptr, alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> &ar, \
                      Args&& ...args) {\
    shm_init(&ar, nullptr, std::forward<Args>(args)...);\
  }

/** Generates the code for destructors */
#define SHM_INHERIT_DESTRUCTOR(CLASS_NAME)\
  ~CLASS_NAME() {\
    if (IsDestructable()) {\
        shm_destroy();\
    }\
  }

/** Generates the code for move operators */
#define SHM_INHERIT_MOVE_OPS(CLASS_NAME, TYPED_CLASS)\
  CLASS_NAME(CLASS_NAME &&other) noexcept {\
    shm_destroy();\
    WeakMove(other);\
  }\
  CLASS_NAME(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar, Allocator *alloc, \
             CLASS_NAME &&other) noexcept {\
    (void) ar; (void) alloc;\
    shm_destroy();\
    WeakMove(other);\
  }\
  CLASS_NAME& operator=(CLASS_NAME &&other) noexcept {\
    if (this != &other) {\
      shm_destroy();\
      WeakMove(other);\
    }\
    return *this;\
  }\
  void shm_init(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar, Allocator *alloc,\
                CLASS_NAME &&other) noexcept {\
    (void) ar; (void) alloc;\
    shm_destroy();\
    WeakMove(other);\
  }\
  void shm_init(CLASS_NAME &&other) noexcept {\
    shm_destroy();\
    WeakMove(other);\
  }

/** Generates the code for copy operators */
#define SHM_INHERIT_COPY_OPS(CLASS_NAME, TYPED_CLASS)\
  CLASS_NAME(const CLASS_NAME &other) noexcept {\
    shm_init(other);\
  }\
  CLASS_NAME(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar, Allocator *alloc, \
             const CLASS_NAME &other) noexcept {\
    (void) ar; (void) alloc;\
    shm_init(other);\
  }\
  CLASS_NAME& operator=(const CLASS_NAME &other) {\
    if (this != &other) {\
      shm_init(other);\
    }\
    return *this;\
  }\
  void shm_init(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar, Allocator *alloc, \
             const CLASS_NAME &other) noexcept {\
    (void) ar; (void) alloc;\
    shm_init(other);\
  }\
  void shm_init(const CLASS_NAME &other) {\
    shm_destroy();\
    StrongCopy(other);\
  }

/**
 * Enables a specific ShmArchive type to be serialized
 * */
#define SHM_SERIALIZE_WRAPPER(AR_TYPE)\
  void operator>>(ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) const {\
    shm_serialize(ar);\
  }

/**
 * Enables a specific ShmArchive type to be deserialized
 * */
#define SHM_DESERIALIZE_WRAPPER(AR_TYPE)\
  void operator<<(                      \
    const labstor::ipc::ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) {\
    shm_deserialize(ar);\
  }

/** Enables serialization + deserialization for data structures */
#define SHM_SERIALIZE_DESERIALIZE_WRAPPER(AR_TYPE)\
  SHM_SERIALIZE_WRAPPER(AR_TYPE)\
  SHM_DESERIALIZE_WRAPPER(AR_TYPE)


}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
