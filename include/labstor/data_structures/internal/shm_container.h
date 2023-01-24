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
#include "shm_deserialize.h"

namespace lipc = labstor::ipc;

namespace labstor::ipc {

/** Bits used for determining how to destroy an object */
/// The container's header has been allocated
#define SHM_CONTAINER_VALID BIT_OPT(uint16_t, 0)
/// The is not empty
#define SHM_CONTAINER_DATA_VALID BIT_OPT(uint16_t, 1)
/// The header was allocated by this container
#define SHM_CONTAINER_HEADER_DESTRUCTABLE BIT_OPT(uint16_t, 2)
/// The container is responsible for destroying data
#define SHM_CONTAINER_DESTRUCTABLE BIT_OPT(uint16_t, 3)

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

  /** Default constructor */
  ShmBaseHeader() = default;

  /** Copy constructor */
  ShmBaseHeader(const ShmBaseHeader &other) {}

  /** Copy assignment operator */
  ShmBaseHeader& operator=(const ShmBaseHeader &other) {
    return *this;
  }

  /**
   * Disable copying of the flag field, as all flags
   * pertain to a particular ShmHeader allocation.
   * */
  void strong_copy(const ShmBaseHeader &other) {}

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

  /** Initialize the data structure's allocator */
  inline void shm_init_allocator(Allocator *alloc) {
    if (IsValid()) { return; }
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /////////////////////
  /// Flags Operations
  /////////////////////

  /** Sets this object as destructable */
  void SetDestructable() {
    flags_.SetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Sets this object as not destructable */
  void UnsetDestructable() {
    flags_.UnsetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if this container is destructable */
  bool IsDestructable() const {
    return flags_.OrBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if container has a valid header */
  bool IsValid() const {
    return flags_.OrBits(SHM_CONTAINER_VALID);
  }

  /** Set container header invalid */
  void UnsetValid() {
    flags_.UnsetBits(SHM_CONTAINER_VALID |
      SHM_CONTAINER_DESTRUCTABLE | SHM_CONTAINER_HEADER_DESTRUCTABLE);
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
