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
 * Implements generic shm_serialize and deserialize methods to use
 * in subsequent data structures.
 * */
template<typename TYPED_HEADER>
class ShmSerializer : public ShmArchiveable {
 protected:
  Pointer header_ptr_;
  Allocator *alloc_;
  TYPED_HEADER *header_;

 public:
  /** Default constructor */
  ShmSerializer()
  : header_ptr_(kNullPointer), alloc_(nullptr), header_(nullptr) {}

  /** Serialize an object into a raw pointer */
  void shm_serialize(Pointer &header_ptr) const {
    header_ptr = header_ptr_;
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr) {
    header_ptr_ = header_ptr;
    if (header_ptr_.is_null()) { return; }
    alloc_ = LABSTOR_MEMORY_MANAGER->GetAllocator(header_ptr_.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->Convert<TYPED_HEADER>(header_ptr_);
  }


};

#define SHM_SERIALIZER_TEMPLATE(TYPED_HEADER)\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::header_ptr_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::alloc_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::header_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::shm_serialize;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::shm_deserialize;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::IsNull;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::SetNull;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::WeakCopy;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::WeakMove;

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
