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
#include "shm_macros.h"
#include "shm_archive.h"

namespace lipc = labstor::ipc;

namespace labstor::ipc {

/** The shared-memory header used for data structures */
template<typename T>
struct ShmHeader;

/** Simplify ShmContainer inheritance */
#define SHM_CONTAINER(TYPED_CLASS) \
  lipc::ShmContainer<ShmHeader<TYPE_UNWRAP(TYPED_CLASS)>>

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_HEADER>
class ShmContainer : public ShmArchiveable {
 protected:
  Pointer header_ptr_;
  Allocator *alloc_;
  TYPED_HEADER *header_;
  bool destructable_;

 public:
  /** Default constructor */
  ShmContainer()
  : header_ptr_(kNullPointer),
    alloc_(nullptr), header_(nullptr), destructable_(true) {}

  /** Set the allocator of the data structure */
  void shm_init_header(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /** Serialize an object into a raw pointer */
  void shm_serialize_header(Pointer &header_ptr) const {
    header_ptr = header_ptr_;
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize_header(const Pointer &header_ptr) {
    header_ptr_ = header_ptr;
    if (header_ptr.is_null()) { return; }
    alloc_ = LABSTOR_MEMORY_MANAGER->GetAllocator(header_ptr.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->
      Convert<TYPED_HEADER>(header_ptr);
  }

  /** Copy only pointers */
  void WeakCopy(const ShmContainer &other) {
    header_ptr_ = other.header_ptr_;
    header_ = other.header_;
    alloc_ = other.alloc_;
    destructable_ = other.destructable_;
  }

  /** Move only pointers */
  void WeakMove(ShmContainer &other) {
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

 public:
  ////////////////////////////////
  ////////REQUIRED METHODS
  ///////////////////////////////

  /** Copy constructor */
  // void StrongCopy(const CLASS_NAME &other);
};

/** Typed nullptr for allocator */
#define SHM_ALLOCATOR_NULL reinterpret_cast<Allocator*>(NULL)

/** Generates the code for constructors  */
#define SHM_INHERIT_CONSTRUCTORS(CLASS_NAME)\
  CLASS_NAME() = default;\
  template<typename ...Args>\
  explicit CLASS_NAME(Args&& ...args) {\
    shm_init_main(SHM_ALLOCATOR_NULL, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(Args&& ...args) {\
    shm_init_main(SHM_ALLOCATOR_NULL, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(alloc, std::forward<Args>(args)...);\
  }

/** Generates the code for destructors  */
#define SHM_INHERIT_DESTRUCTORS(CLASS_NAME)\
  ~CLASS_NAME() {\
    if (destructable_) {\
      shm_destroy();\
    }\
  }

/** Generates the code for move operators */
#define SHM_INHERIT_MOVE_OPS(CLASS_NAME)\
  CLASS_NAME(CLASS_NAME &&other) noexcept {\
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
  void shm_init(CLASS_NAME &&other) noexcept {\
    shm_destroy();\
    WeakMove(other);\
  }

/** Generates the code for copy operators */
#define SHM_INHERIT_COPY_OPS(CLASS_NAME)\
  CLASS_NAME(const CLASS_NAME &other) noexcept {\
    shm_destroy();\
    shm_init(other);\
  }\
  CLASS_NAME& operator=(const CLASS_NAME &other) {\
    if (this != &other) {\
      shm_destroy();\
      shm_init(other);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::Allocator *alloc, const CLASS_NAME &other) {\
    shm_destroy();\
    StrongCopy(other);\
  }

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_CONTAINER_USING_NS(TYPED_HEADER)\
  using labstor::ipc::ShmContainer<TYPE_UNWRAP(TYPED_HEADER)>

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
#define SHM_CONTAINER_TEMPLATE_X(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::header_ptr_;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::alloc_;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::header_;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::destructable_;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::shm_init_header;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::shm_serialize;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::shm_deserialize;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::IsNull;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::SetNull;            \
SHM_CONTAINER_USING_NS(TYPED_HEADER)::SetDestructable;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::UnsetDestructable;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::WeakCopy;\
SHM_CONTAINER_USING_NS(TYPED_HEADER)::WeakMove;\
SHM_INHERIT_CONSTRUCTORS(CLASS_NAME)\
SHM_INHERIT_DESTRUCTORS(CLASS_NAME)\
SHM_INHERIT_MOVE_OPS(CLASS_NAME)\
SHM_INHERIT_COPY_OPS(CLASS_NAME)\
SHM_SERIALIZE_DESERIALIZE_OPS(TYPED_CLASS)

/**
 * ShmContainers should define:
 * CLASS_NAME and TYPED_CLASS macros and then
 * unset them in their respective header files.
 * */

#define SHM_CONTAINER_TEMPLATE(CLASS_NAME, TYPED_CLASS) \
  SHM_CONTAINER_TEMPLATE_X(CLASS_NAME, TYPED_CLASS, \
                           TYPE_WRAP(ShmHeader<TYPE_UNWRAP(TYPED_CLASS)>))

/**
 * ShmContainers should define:
 * CLASS_NAME and TYPED_CLASS macros and then
 * unset them in their respective header files.
 * */

#define BASIC_SHM_CONTAINER_TEMPLATE \
  SHM_CONTAINER_TEMPLATE(CLASS_NAME, TYPE_WRAP(TYPED_CLASS))

}  // namespace labstor::ipc

#endif  // LABSTOR_SHM_CONTAINER_H_
