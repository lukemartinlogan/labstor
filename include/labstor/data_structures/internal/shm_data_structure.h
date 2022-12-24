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

/**
 * ShmDataStructures all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_HEADER>
class ShmDataStructure : public ShmArchiveable {
 protected:
  Pointer header_ptr_;
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;
  Allocator *alloc_;
  TYPED_HEADER *header_;
  bool destructable_;

 public:
  /** Default constructor */
  ShmDataStructure()
  : header_ptr_(kNullPointer), mem_mngr_(LABSTOR_MEMORY_MANAGER),
    alloc_(nullptr), header_(nullptr), destructable_(true) {}

  /** Set the allocator of the data structure */
  void shm_init(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /** Serialize an object into a raw pointer */
  void shm_serialize(Pointer &header_ptr) const {
    header_ptr = header_ptr_;
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr) {
    header_ptr_ = header_ptr;
    if (header_ptr.is_null()) { return; }
    alloc_ = mem_mngr_->GetAllocator(header_ptr.allocator_id_);
    header_ = mem_mngr_->Convert<TYPED_HEADER>(header_ptr);
  }

  /** Copy only pointers */
  void WeakCopy(const ShmDataStructure &other) {
    header_ptr_ = other.header_ptr_;
    header_ = other.header_;
    alloc_ = other.alloc_;
    destructable_ = other.destructable_;
  }

  /** Move only pointers */
  void WeakMove(ShmDataStructure &other) {
    header_ptr_ = std::move(other.header_ptr_);
    header_ = std::move(other.header_);
    alloc_ = other.alloc_;
    destructable_ = other.destructable_;
    other.SetNull();
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    destructable_ = true;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    destructable_ = false;
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
  using labstor::ipc::ShmDataStructure<TYPE_UNWRAP(TYPED_HEADER)>

#define SHM_DATA_STRUCTURE_TEMPLATE(TYPED_HEADER)\
SHM_DATA_STRUCTURE_USING_NS::header_ptr_;\
SHM_DATA_STRUCTURE_USING_NS::alloc_;\
SHM_DATA_STRUCTURE_USING_NS::header_;\
SHM_DATA_STRUCTURE_USING_NS::mem_mngr_;\
SHM_DATA_STRUCTURE_USING_NS::destructable_;\
SHM_DATA_STRUCTURE_USING_NS::shm_serialize;\
SHM_DATA_STRUCTURE_USING_NS::shm_deserialize;\
SHM_DATA_STRUCTURE_USING_NS::IsNull;\
SHM_DATA_STRUCTURE_USING_NS::SetNull;            \
SHM_DATA_STRUCTURE_USING_NS::SetDestructable;\
SHM_DATA_STRUCTURE_USING_NS::UnsetDestructable;\
SHM_DATA_STRUCTURE_USING_NS::WeakCopy;\
SHM_DATA_STRUCTURE_USING_NS::WeakMove;

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
