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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
#define LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_archive.h"

namespace labstor::ipc {

#define IS_SHM_DESTRUCTABLE 0x1
#define IS_SHM_HEADER_DESTRUCTABLE 0x2

/**
 * Data required by all ShmArchives inheriting from ShmDataStructure
 * */
struct ShmDataStructureArchive {
  int flags_;
  Pointer header_ptr_;
};

/**
 * ShmDataStructures all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_CLASS>
class ShmDataStructure : public ShmArchiveable {
 protected:
  Pointer header_ptr_;
  Allocator *alloc_;
  ShmArchive<TYPED_CLASS> *header_;
  int flags_;

 public:
  /** Default constructor */
  ShmDataStructure() : header_ptr_(kNullPointer) {}

  /** Set the allocator of the data structure */
  void shm_init(ShmArchive<TYPED_CLASS> *ar, Allocator *alloc) {
    flags_ = 0;
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
    if (ar == nullptr) {
      header_ = alloc_->template
          AllocateConstructObjs<ShmArchive<TYPED_CLASS>>(1, header_ptr_);
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
  void shm_serialize(ShmDataStructureArchive &ar) const {
    ar.header_ptr_ = header_ptr_;
    ar.flags_ = flags_;
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const ShmDataStructureArchive &ar) {
    header_ptr_ = ar.header_ptr_;
    if (IsNull()) { return; }
    alloc_ = LABSTOR_MEMORY_MANAGER->
      GetAllocator(header_ptr_.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->
      Convert<ShmArchive<TYPED_CLASS>>(header_ptr_);
    flags_ = header_->flags_;
  }

  /** Copy only pointers */
  void WeakCopy(const ShmDataStructure &other) {
    header_ptr_ = other.header_ptr_;
    header_ = other.header_;
    alloc_ = other.alloc_;
    flags_ = other.flags_;
  }

  /** Move only pointers */
  void WeakMove(ShmDataStructure &other) {
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
};

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_DATA_STRUCTURE_USING_NS\
  using labstor::ipc::ShmDataStructure<TYPE_UNWRAP(TYPED_CLASS)>

#define SHM_DATA_STRUCTURE_TEMPLATE(TYPED_CLASS)\
SHM_DATA_STRUCTURE_USING_NS::header_ptr_;\
SHM_DATA_STRUCTURE_USING_NS::alloc_;\
SHM_DATA_STRUCTURE_USING_NS::header_;\
SHM_DATA_STRUCTURE_USING_NS::shm_serialize;\
SHM_DATA_STRUCTURE_USING_NS::shm_deserialize;\
SHM_DATA_STRUCTURE_USING_NS::IsNull;\
SHM_DATA_STRUCTURE_USING_NS::SetNull;            \
SHM_DATA_STRUCTURE_USING_NS::SetDestructable;\
SHM_DATA_STRUCTURE_USING_NS::IsDestructable;\
SHM_DATA_STRUCTURE_USING_NS::UnsetDestructable; \
SHM_DATA_STRUCTURE_USING_NS::SetHeaderDestructable;\
SHM_DATA_STRUCTURE_USING_NS::IsHeaderDestructable;\
SHM_DATA_STRUCTURE_USING_NS::UnsetHeaderDestructable;\
SHM_DATA_STRUCTURE_USING_NS::WeakCopy;\
SHM_DATA_STRUCTURE_USING_NS::WeakMove;

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
