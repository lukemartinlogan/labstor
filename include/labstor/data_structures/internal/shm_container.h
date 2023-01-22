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


#ifndef LABSTOR_SHM_CONTAINER_H_
#define LABSTOR_SHM_CONTAINER_H_

#include "labstor/memory/memory_manager.h"
#include "labstor/constants/macros.h"
#include "shm_container_macro.h"
#include "shm_macros.h"
#include "shm_archive.h"
#include "shm_ref.h"

namespace lipc = labstor::ipc;

namespace labstor::ipc {

/** Bits used for determining how to destroy an object */
#define SHM_CONTAINER_VALID BIT_OPT(uint16_t, 0)
#define SHM_CONTAINER_DATA_VALID BIT_OPT(uint16_t, 1)
#define SHM_CONTAINER_HEADER_DESTRUCTABLE BIT_OPT(uint16_t, 2)
#define SHM_CONTAINER_DESTRUCTABLE BIT_OPT(uint16_t, 3)

/** Typed nullptr for allocator */
#define SHM_ALLOCATOR_NULL reinterpret_cast<lipc::Allocator*>(NULL)

/** Typed nullptr for TypedPointer */
#define SHM_ARCHIVE_NULL(TYPED_CLASS) \
  reinterpret_cast<lipc::TypedPointer<TYPE_UNWRAP(TYPED_CLASS)>*>(NULL)

/** The shared-memory header used for data structures */
template<typename T>
struct ShmHeader;

/**
 * Indicates that a type is internal to a container, and needs to be
 * passed an Allocator to its shm_destroy.
 * */
struct ShmContainerEntry {};

/** The base ShmHeader used for all containers */
struct ShmBaseHeader {
  bitfield32_t flags_;

  ShmBaseHeader() = default;

  /**
   * Disable copying of the flag field, as all flags
   * pertain to a particular ShmHeader allocation.
   * */
  ShmBaseHeader(const ShmBaseHeader &other) = delete;

  /** Publicize bitfield operations */
  INHERIT_BITFIELD_OPS(flags_, uint16_t)
};

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a TypedPointer.
 * */
class ShmContainer : public ShmArchiveable {
 public:
  Allocator *alloc_;
  bitfield32_t flags_;

 public:

  /////////////////////
  /// Constructors
  /////////////////////

  /** Default constructor (flags are cleared) */
  ShmContainer() = default;

  /**
   * Initialize the data structure's allocator
   * */
  inline void shm_init_allocator(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /////////////////////
  /// Query Operations
  /////////////////////

  /** Get the allocator for this container */
  Allocator* GetAllocator() {
    return alloc_;
  }

  /** Get the allocator for this container */
  Allocator* GetAllocator() const {
    return alloc_;
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const {
    return alloc_->GetId();
  }
};

/** Typed nullptr */
template<typename T>
static inline T* typed_nullptr() {
  return reinterpret_cast<T*>(NULL);
}

}  // namespace labstor::ipc

#endif  // LABSTOR_SHM_CONTAINER_H_
