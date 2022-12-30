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

#include "labstor/constants/macros.h"
#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_archive.h"

namespace labstor::ipc {

#define SHM_CONTAINER_IS_INITIALIZED 0x1

/**
 * Data required by all ShmArchives inheriting from ShmContainer
 * */
struct ShmContainerArchive {
  int flags_;
  allocator_id_t allocator_id_;

  ShmContainerArchive() = default;

  ShmContainerArchive(int flags) : flags_(flags) {}
};

/**
 * Indicates that the object should be moved,
 * as opposed to copied.
 * */
template<typename TYPED_CLASS>
struct ShmMove {
  TYPED_CLASS &obj_;
  ShmMove(TYPED_CLASS &obj)
    : obj_(obj) {}
};

/**
 * Indicates that the object should be copied,
 * as opposed to moved.
 * */
template<typename TYPED_CLASS>
struct ShmCopy {
  TYPED_CLASS &obj_;
  ShmCopy(TYPED_CLASS &obj)
    : obj_(obj) {}
};

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_CLASS>
class ShmContainer : public ShmArchiveable {
 protected:
  Allocator *alloc_;
  ShmArchive<TYPED_CLASS> *header_;

 public:
  /** Default constructor */
  ShmContainer() : header_(nullptr) {}

  /** Set the allocator of the data structure */
  void shm_init_header(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
    SetInitialized();
  }

  /** Serialize an object into a raw pointer */
  void shm_serialize(ShmContainerArchive &ar) const {
    ar.allocator_id_ = GetAllocatorId();
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const ShmContainerArchive &ar) {
    alloc_ = LABSTOR_MEMORY_MANAGER->
      GetAllocator(header_->allocator_id_);
    header_ = ar;
  }

  /** Copy only pointers */
  void WeakCopy(const ShmContainer &other) {
    header_ = other.header_;
    alloc_ = other.alloc_;
  }

  /** Move only pointers */
  void WeakMove(ShmContainer &other) {
    header_ = other.header_;
    alloc_ = other.alloc_;
    other.SetNull();
  }

  /** Set as initialized */
  void SetInitialized() {
    header_->flags_ |= SHM_CONTAINER_IS_INITIALIZED;
  }

  /** Set to null */
  void SetNull() {
    header_->flags_ &= ~SHM_CONTAINER_IS_INITIALIZED;
  }

  /** Check if null */
  bool IsNull() const {
    return header_->flags_ & SHM_CONTAINER_IS_INITIALIZED;
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

/** Namespace simplification */
namespace lipc = labstor::ipc;

/** Wrappers around null pointers to help with template deduction */
#define SHM_ALLOCATOR_NULL reinterpret_cast<lipc::Allocator*>(NULL)

/** Generates the code for constructors */
#define SHM_INHERIT_CONSTRUCTOR(CLASS_NAME, TYPED_CLASS)\
  template<typename ...Args>\
  explicit CLASS_NAME(Args&& ...args) {\
    shm_init_main(SHM_ALLOCATOR_NULL, \
                  std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> &ar) {\
      shm_deserialize(ar);\
  }\
  template<typename ...Args>\
  void shm_init(Args&& ...args) {\
    shm_init_main(SHM_ALLOCATOR_NULL,\
    std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>

/** Generates the code for destructors */
#define SHM_INHERIT_DESTRUCTOR(CLASS_NAME)\
  ~CLASS_NAME() {\
    shm_destroy();
  }

/** Generates the code for move operators */
#define SHM_INHERIT_MOVE_OPS(CLASS_NAME, TYPED_CLASS)\
  CLASS_NAME(CLASS_NAME &&other) {\
    WeakMove(SHM_ALLOCATOR_NULL, other);\
  }\
  CLASS_NAME& operator=(CLASS_NAME &&other) noexcept {\
    if (this != &other) {\
      WeakMove(SHM_ALLOCATOR_NULL, other);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::Allocator *alloc, \
                     CLASS_NAME &&other) noexcept {\
    WeakMove(alloc, other);\
  }\
  explicit CLASS_NAME(lipc::Allocator *alloc, \
                      lipc::ShmMove<TYPE_UNWRAP(TYPED_CLASS)> other) {\
    WeakMove(alloc, other.obj_);\
  }\
  CLASS_NAME& operator=(lipc::ShmMove<TYPE_UNWRAP(TYPED_CLASS)> other) \
  noexcept {\
    if (this != &other.obj_) {\
      WeakMove(SHM_ALLOCATOR_NULL, other.obj_);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::Allocator *alloc, \
                     lipc::ShmMove<CLASS_NAME> other) noexcept {\
    WeakMove(alloc, other.obj_);\
  }

/** Generates the code for copy operators */
#define SHM_INHERIT_COPY_OPS(CLASS_NAME, TYPED_CLASS)\
  CLASS_NAME(const CLASS_NAME &other) {\
    StrongCopy(SHM_ALLOCATOR_NULL, other);\
  }\
  CLASS_NAME& operator=(const CLASS_NAME &other) {\
    if (this != &other) {\
      StrongCopy(SHM_ALLOCATOR_NULL, other);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::Allocator *alloc, \
                     const CLASS_NAME &other) noexcept {\
    StrongCopy(alloc, other);\
  }\
  explicit CLASS_NAME(lipc::Allocator *alloc, \
                      lipc::ShmCopy<TYPE_UNWRAP(TYPED_CLASS)> other) {\
    StrongCopy(alloc, other.obj_);\
  }\
  CLASS_NAME& operator=(lipc::ShmCopy<TYPE_UNWRAP(TYPED_CLASS)> other) \
  noexcept {\
    if (this != &other.obj_) {\
      StrongCopy(SHM_ALLOCATOR_NULL, other.obj_);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::Allocator *alloc, \
                     lipc::ShmCopy<CLASS_NAME> other) noexcept {\
    StrongCopy(alloc, other.obj_);\
  }

/**
 * Macros for simplifying shm_destroy
 * */
#define SHM_DESTROY_PRIOR\
  if (IsNull()) { return; }

#define SHM_DESTROY_AFTER\
  SetNull();

/**
 * Macros for simplifying StrongCopy
 * */
#define SHM_STRONG_COPY_CONSTRUCT_M(...)\
  if (alloc == nullptr) { alloc = other.alloc_; }\
  shm_init_main(alloc, __VA_ARGS__);

#define SHM_STRONG_COPY_RECONSTRUCT_M(...)\
  shm_destroy();\
  shm_init_main(alloc_, __VA_ARGS__);

#define SHM_STRONG_COPY_CONSTRUCT_E()\
  if (alloc == nullptr) { alloc = other.alloc_; }\
  shm_init_main(alloc);

#define SHM_STRONG_COPY_RECONSTRUCT_E()\
  shm_destroy();\
  shm_init_main(alloc_);

/**
 * Macros for simplifying WeakMove
 * */
#define SHM_WEAK_MOVE_CONSTRUCT_M(...)\
  SHM_STRONG_COPY_CONSTRUCT_M(__VA_ARGS__)

#define SHM_WEAK_MOVE_RECONSTRUCT_M(...) \
  SHM_STRONG_COPY_RECONSTRUCT_M(__VA_ARGS__)

#define SHM_WEAK_MOVE_CONSTRUCT_E() SHM_STRONG_COPY_CONSTRUCT_E()

#define SHM_WEAK_MOVE_RECONSTRUCT_E() SHM_STRONG_COPY_RECONSTRUCT_E()

#define SHM_DEFAULT_WEAK_MOVE(TYPED_CLASS)\
  void WeakMove(ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar,\
                Allocator *alloc,\
                TYPE_UNWRAP(TYPED_CLASS) &other) {\
    if (IsNull()) {\
      SHM_WEAK_MOVE_CONSTRUCT_E()\
    } else {\
      SHM_WEAK_MOVE_RECONSTRUCT_E()\
    }\
    ShmContainer<TYPE_UNWRAP(TYPED_CLASS)>::WeakMove(other);\
  }

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
SHM_CONTAINER_USING_NS_::shm_init_header;\
SHM_CONTAINER_USING_NS_::shm_serialize;\
SHM_CONTAINER_USING_NS_::shm_deserialize;\
SHM_CONTAINER_USING_NS_::IsNull;\
SHM_CONTAINER_USING_NS_::SetNull;\
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
