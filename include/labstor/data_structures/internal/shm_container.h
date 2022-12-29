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


#ifndef LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_H_
#define LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_archive.h"

namespace labstor::ipc {

#define IS_SHM_DESTRUCTABLE 0x1
#define IS_SHM_HEADER_DESTRUCTABLE 0x2

/**
 * Data required by all ShmArchives inheriting from ShmContainer
 * */
struct ShmContainerArchive {
  int flags_;
  Pointer header_ptr_;
};

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_CLASS>
class ShmContainer : public ShmArchiveable {
 protected:
  Pointer header_ptr_;
  Allocator *alloc_;
  ShmArchive<TYPED_CLASS> *header_;
  int flags_;

 public:
  /** Default constructor */
  ShmContainer() : header_ptr_(kNullPointer) {}

  /** Set the allocator of the data structure */
  template<typename ...Args>
  void shm_init(ShmArchive<TYPED_CLASS> *ar, Allocator *alloc, Args&& ...args) {
    flags_ = 0;
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
    if (ar == nullptr) {
      header_ = alloc_->template
          AllocateConstructObjs<ShmArchive<TYPED_CLASS>>(
            1, header_ptr_, std::forward<Args>(args)...);
      SetHeaderDestructable();
    } else {
      header_ = ar;
      header_ptr_ = alloc_->template
        Convert<ShmArchive<TYPED_CLASS>>(header_);
      UnsetHeaderDestructable();
    }
    SetDestructable();
  }

  /** Serialize an object into a raw pointer */
  void shm_serialize(ShmContainerArchive &ar) const {
    ar.header_ptr_ = header_ptr_;
    ar.flags_ = flags_;
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const ShmContainerArchive &ar) {
    header_ptr_ = ar.header_ptr_;
    if (IsNull()) { return; }
    alloc_ = LABSTOR_MEMORY_MANAGER->
      GetAllocator(header_ptr_.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->
      Convert<ShmArchive<TYPED_CLASS>>(header_ptr_);
    flags_ = header_->flags_;
  }

  /** Copy only pointers */
  void WeakCopy(const ShmContainer &other) {
    header_ptr_ = other.header_ptr_;
    header_ = other.header_;
    alloc_ = other.alloc_;
    flags_ = other.flags_;
  }

  /** Move only pointers */
  void WeakMove(ShmContainer &other) {
    header_ptr_ = std::move(other.header_ptr_);
    header_ = std::move(other.header_);
    alloc_ = other.alloc_;
    flags_ = other.flags_;
    other.SetNull();
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    flags_ |= IS_SHM_DESTRUCTABLE;
  }

  /** Determines if this object is destructable */
  bool IsDestructable() {
    return flags_ & IS_SHM_DESTRUCTABLE;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    flags_ &= ~IS_SHM_DESTRUCTABLE;
  }

  /** Sets this object's header as destructable */
  void SetHeaderDestructable() {
    flags_ |= IS_SHM_HEADER_DESTRUCTABLE;
    header_->flags_ |= IS_SHM_HEADER_DESTRUCTABLE;
  }

  /** Determines if this object is destructable */
  bool IsHeaderDestructable() {
    return flags_ & IS_SHM_HEADER_DESTRUCTABLE;
  }

  /** Sets this object as nondestructable */
  void UnsetHeaderDestructable() {
    flags_ &= ~IS_SHM_HEADER_DESTRUCTABLE;
    header_->flags_ &= IS_SHM_HEADER_DESTRUCTABLE;
  }

  /** Set to null */
  void SetNull() {
    header_ptr_.set_null();
  }

  /** Check if null */
  bool IsNull() const {
    return header_ptr_.is_null();
  }

  /** Get the allocator for this pointer */
  Allocator* GetAllocator() {
    return alloc_;
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const {
    return alloc_->GetId();
  }

 public:
  ////////////////////////////////
  ////////REQUIRED METHODS
  ///////////////////////////////

  /** Copy constructor */
  // void StrongCopy(const CLASS_NAME &other);
};

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_CONTAINER_USING_NS_\
  using labstor::ipc::ShmContainer<TYPE_UNWRAP(TYPED_CLASS)>

/**
 * Define various functions and variables common across all
 * SharedMemoryDataStructures.
 *
 * Variables which derived classes should see are not by default visible
 * due to the nature of c++ template resolution.
 *
 * 1. Create Move constructors + Move assignment operators.
 * 2. Create Copy constructors + Copy assignment operators.
 * 3. Create shm_serialize and shm_deserialize for archiving data structures.
 * */
#define SHM_CONTAINER_TEMPLATE(CLASS_NAME, TYPED_CLASS)\
SHM_CONTAINER_USING_NS_::header_ptr_;\
SHM_CONTAINER_USING_NS_::alloc_;\
SHM_CONTAINER_USING_NS_::header_;\
SHM_CONTAINER_USING_NS_::shm_serialize;\
SHM_CONTAINER_USING_NS_::shm_deserialize;\
SHM_CONTAINER_USING_NS_::IsNull;\
SHM_CONTAINER_USING_NS_::SetNull;            \
SHM_CONTAINER_USING_NS_::SetDestructable;\
SHM_CONTAINER_USING_NS_::IsDestructable;\
SHM_CONTAINER_USING_NS_::UnsetDestructable; \
SHM_CONTAINER_USING_NS_::SetHeaderDestructable;\
SHM_CONTAINER_USING_NS_::IsHeaderDestructable;\
SHM_CONTAINER_USING_NS_::UnsetHeaderDestructable;\
SHM_CONTAINER_USING_NS_::WeakCopy;\
SHM_CONTAINER_USING_NS_::WeakMove;\
SHM_INHERIT_CONSTRUCTOR(CLASS_NAME, TYPED_CLASS)\
SHM_INHERIT_DESTRUCTOR(CLASS_NAME)\
SHM_INHERIT_MOVE_OPS(CLASS_NAME, TYPED_CLASS)\
SHM_INHERIT_COPY_OPS(CLASS_NAME, TYPED_CLASS)\
SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)

/**
 * ShmContainers should define:
 * CLASS_NAME and TYPED_CLASS macros and then
 * unset them in their respective header files.
 * */

#define BASIC_SHM_CONTAINER_TEMPLATE \
  SHM_CONTAINER_TEMPLATE(CLASS_NAME, TYPE_WRAP(TYPED_CLASS))

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_H_
