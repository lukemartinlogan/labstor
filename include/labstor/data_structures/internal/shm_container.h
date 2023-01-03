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

/** Bits used for determining how to destroy an object */
#define SHM_CONTAINER_VALID BIT_OPT(uint16_t, 0)
#define SHM_CONTAINER_DATA_VALID BIT_OPT(uint16_t, 1)
#define SHM_CONTAINER_HEADER_DESTRUCTABLE BIT_OPT(uint16_t, 2)
#define SHM_CONTAINER_DESTRUCTABLE BIT_OPT(uint16_t, 3)

/** The shared-memory header used for data structures */
template<typename T>
struct ShmHeader;

/**
 * A reference to a shared-memory object are a simple object
 * stored in shared-memory.
 * */
template<typename T>
struct Ref {
  typedef SHM_T_OR_PTR_T(T) T_Ptr;
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  T_Ptr obj_;

  explicit Ref(T_Ar &other) {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other);
    } else {
      obj_ = &other;
    }
  }

  Ref(const Ref &other) {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other.obj_.ar_);
    } else {
      obj_ = other.obj_;
    }
  }

  Ref(Ref &&other) noexcept {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other.obj_.ar_);
    } else {
      obj_ = other.obj_;
    }
  }

  T& get_ref() {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      return obj_;
    } else {
      return *obj_;
    }
  }

  const T& get_ref_const() {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      return obj_;
    } else {
      return *obj_;
    }
  }

  T& operator*() {
    return get_ref();
  }

  const T& operator*() const {
    return get_ref_const();
  }
};

/** Force a StrongCopy of a container to occur */
template<typename T>
struct Copy {
  const T &obj_;
  Copy(const T &other) : obj_(other) {}
};

/** Force a WeakMove of a container to occur */
template<typename T>
struct Move {
  T &obj_;
  Move(const T &other) : obj_(other) {}
};

/** The base ShmHeader used for all containers */
struct ShmBaseHeader {
  bitfield16_t flags_;

  ShmBaseHeader() = default;

  /**
   * Disable copying of the flag field, as all flags
   * pertain to a particular ShmHeader allocation.
   * */
  ShmBaseHeader(const ShmBaseHeader &other) = delete;

  /** Publicize bitfield operations */
  INHERIT_BITFIELD_OPS(flags_, uint16_t)
};

/** Simplify ShmContainer inheritance */
#define SHM_CONTAINER(TYPED_CLASS) \
  lipc::ShmContainer<TYPE_UNWRAP(TYPED_CLASS), \
    ShmHeader<TYPE_UNWRAP(TYPED_CLASS)>>

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a ShmArchive.
 * */
template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmContainer : public ShmArchiveable {
 protected:
  ShmArchive<TYPED_CLASS> ar_;
  Allocator *alloc_;
  TYPED_HEADER *header_;
  bitfield16_t flags_;

 public:
  /** Default constructor (flags are cleared) */
  ShmContainer() = default;

  /**
   * Initialize a data structure's header + allocator.
   * A container will never re-set or re-allocate its header once it has
   * been set the first time.
   * */
  template<typename ...Args>
  void shm_init_header(ShmArchive<TYPED_CLASS> *ar,
                       Allocator *alloc,
                       Args&& ...args) {
    if (alloc == nullptr) {
      alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }

    if (IsValid()) {
      // The container already has a valid header
      header_->SetBits(SHM_CONTAINER_DATA_VALID);
    } else if (ar == nullptr) {
      // Allocate and initialize a new header
      header_ = alloc_->template
        AllocateConstructObjs<TYPED_HEADER>(
          1, ar_.header_ptr_,
          std::forward<Args>(args)...);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID |
        SHM_CONTAINER_HEADER_DESTRUCTABLE);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
        SHM_CONTAINER_DESTRUCTABLE);
    } else {
      // Initialize the input header
      ar_.header_ptr_ = ar->header_ptr_;
      Allocator::ConstructObj<TYPED_HEADER>(
        *header_, std::forward<Args>(args)...);
      header_ = LABSTOR_MEMORY_MANAGER->template
        Convert<TYPED_HEADER>(ar->header_ptr_);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
        SHM_CONTAINER_DESTRUCTABLE);
    }
  }

  /** Serialize an object into a raw pointer */
  void shm_serialize_header(Pointer &header_ptr) const {
    header_ptr = ar_.header_ptr_;
  }

  /** Deserialize object from a raw pointer */
  bool shm_deserialize_header(const Pointer &header_ptr) {
    ar_.header_ptr_ = header_ptr;
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
    if (ar_.header_ptr_.IsNull()) { return false; }
    alloc_ = LABSTOR_MEMORY_MANAGER->GetAllocator(header_ptr.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->
      Convert<TYPED_HEADER>(header_ptr);
    flags_.SetBits(SHM_CONTAINER_VALID);
    return true;
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    flags_.SetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Sets this object as indestructable */
  void UnsetDestructable() {
    flags_.UnsetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if this container is destructable */
  bool IsDestructable() {
    return flags_.CheckBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if container has a valid header */
  bool IsValid() const {
    return flags_.CheckBits(SHM_CONTAINER_VALID);
  }

  /** Check if header's data is valid */
  bool IsDataValid() const {
    return header_->CheckBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Set container header invalid */
  void UnsetValid() {
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if header's data is valid */
  void UnsetDataValid() const {
    header_->UnsetBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Check if null */
  bool IsNull() const {
    return !IsValid() || !IsDataValid();
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
#define SHM_ALLOCATOR_NULL reinterpret_cast<lipc::Allocator*>(NULL)

/** Typed nullptr for ShmArchive */
#define SHM_ARCHIVE_NULL(TYPED_CLASS) \
  reinterpret_cast<lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)>*>(NULL)

/** Generates the code for constructors  */
#define SHM_INHERIT_CONSTRUCTORS(CLASS_NAME, TYPED_CLASS)\
  template<typename ...Args>\
  explicit CLASS_NAME(Args&& ...args) {\
    shm_init_main(SHM_ARCHIVE_NULL(TYPED_CLASS), \
                  SHM_ALLOCATOR_NULL,  \
                  std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(SHM_ARCHIVE_NULL(TYPED_CLASS), alloc,  \
        std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> &ar,\
                      lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(&ar, alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  explicit CLASS_NAME(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> &ar) {\
    shm_deserialize(ar);\
  }\
  template<typename ...Args>\
  void shm_init(Args&& ...args) {\
    shm_init_main(SHM_ARCHIVE_NULL(TYPED_CLASS), SHM_ALLOCATOR_NULL,  \
                  std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(SHM_ARCHIVE_NULL(TYPED_CLASS),  \
                  alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> &ar,\
                lipc::Allocator *alloc, Args&& ...args) {\
    shm_init_main(&ar, alloc, std::forward<Args>(args)...);\
  }\
  template<typename ...Args>\
  void shm_init(lipc::Ref<TYPE_UNWRAP(TYPED_CLASS)> &ref) {\
    shm_deserialize(ref.obj_.ar_);\
  }

/** Generates the code for destructors  */
#define SHM_INHERIT_DESTRUCTORS(CLASS_NAME)\
  ~TYPE_UNWRAP(CLASS_NAME)() {\
    if (IsDestructable()) {\
      shm_destroy(true);\
    }\
  }

/** Generates the code for move operators */
#define SHM_INHERIT_MOVE_OPS(CLASS_NAME)\
  explicit TYPE_UNWRAP(CLASS_NAME)(TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
    WeakMove(other);\
  }\
  TYPE_UNWRAP(CLASS_NAME)& operator=(   \
      TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
    if (this != &other) {\
      WeakMove(other);\
    }\
    return *this;\
  }\
  void shm_init(TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
    WeakMove(other);\
  }\
  TYPE_UNWRAP(CLASS_NAME)& operator=(                \
      const lipc::Move<TYPE_UNWRAP(CLASS_NAME)> &other) {\
    WeakMove(other.obj_);\
    return *this;\
  }\
  void shm_init_main(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar,\
        lipc::Allocator *alloc, \
        lipc::Move<TYPE_UNWRAP(CLASS_NAME)> &other) {\
    WeakMove(other.obj_);\
  }

/** Generates the code for copy operators */
#define SHM_INHERIT_COPY_OPS(TYPED_CLASS, CLASS_NAME)\
  TYPE_UNWRAP(CLASS_NAME)(const TYPE_UNWRAP(CLASS_NAME) &other) noexcept {\
    shm_init(other);\
  }\
  TYPE_UNWRAP(CLASS_NAME)& operator=(const TYPE_UNWRAP(CLASS_NAME) &other) {\
    if (this != &other) {\
      StrongCopy(other);\
    }\
    return *this;\
  }\
  void shm_init_main(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar,\
        lipc::Allocator *alloc, \
        const TYPE_UNWRAP(CLASS_NAME) &other) {\
    StrongCopy(other);\
  }\
  TYPE_UNWRAP(CLASS_NAME)& operator=(                \
      const lipc::Copy<TYPE_UNWRAP(CLASS_NAME)> &other) {\
    StrongCopy(other.obj_);\
    return *this;\
  }\
  void shm_init_main(lipc::ShmArchive<TYPE_UNWRAP(TYPED_CLASS)> *ar,\
        lipc::Allocator *alloc, \
        lipc::Copy<TYPE_UNWRAP(CLASS_NAME)> &other) {\
    StrongCopy(other.obj_);\
  }

/**
 * Simplify shm_destroy
 * */
#define SHM_DESTROY_DATA_START\
  if (!IsValid()) { return; } \
  if (IsDataValid()) {
#define SHM_DESTROY_DATA_END\
  }\
  UnsetDataValid();
#define SHM_DESTROY_END \
  if (destroy_header && \
      header_->CheckBits(SHM_CONTAINER_HEADER_DESTRUCTABLE)) {\
    auto alloc = LABSTOR_MEMORY_MANAGER->\
      GetAllocator(ar_.header_ptr_.allocator_id_);\
    alloc->Free(ar_.header_ptr_);\
    UnsetValid();\
  }

/** Simplify WeakMove + StrongCopy */
#define SHM_WEAK_COPY_START()\
  shm_destroy(false);\
  if (other.IsNull()) { return; }\
  alloc_ = other.alloc_;
#define SHM_WEAK_COPY_END\
  header_->SetBits(SHM_CONTAINER_DATA_VALID);

/** Simplify WeakMove */
#define SHM_WEAK_MOVE_START(...) \
  SHM_WEAK_COPY_START()\
  shm_init_header(__VA_ARGS__);
#define SHM_WEAK_MOVE_END()\
  if (!IsDestructable() || !other.IsDestructable()) {\
    UnsetDestructable();\
  }\
  other.UnsetDataValid();\
  other.shm_destroy(true);\
  SHM_WEAK_COPY_END
#define SHM_WEAK_MOVE_DEFAULT(TYPED_CLASS) \
  SHM_ARCHIVE_NULL(TYPED_CLASS), other.alloc_

/** Simplify StrongCopy */
#define SHM_STRONG_COPY_START(...) \
  SHM_WEAK_COPY_START()\
  shm_init_main(__VA_ARGS__);
#define SHM_STRONG_COPY_END() \
  SHM_WEAK_COPY_END
#define SHM_STRONG_COPY_DEFAULT(TYPED_CLASS)\
  SHM_ARCHIVE_NULL(TYPED_CLASS), other.alloc_

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)\
  using labstor::ipc::ShmContainer<TYPE_UNWRAP(TYPED_CLASS), \
    TYPE_UNWRAP(TYPED_HEADER)>

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
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::ar_;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::alloc_;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::header_;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::flags_;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::shm_init_header;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::shm_serialize_header;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::shm_deserialize_header;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::IsDataValid;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::IsValid;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::IsNull;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::UnsetValid;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::UnsetDataValid;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::SetDestructable;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::UnsetDestructable;\
SHM_CONTAINER_USING_NS(TYPED_CLASS, TYPED_HEADER)::IsDestructable;\
SHM_INHERIT_CONSTRUCTORS(CLASS_NAME, TYPED_CLASS)\
SHM_INHERIT_DESTRUCTORS(CLASS_NAME)\
SHM_INHERIT_MOVE_OPS(CLASS_NAME)\
SHM_INHERIT_COPY_OPS(TYPED_CLASS, CLASS_NAME)\
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
