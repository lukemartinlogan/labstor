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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_STRUCT_H_
#define LABSTOR_DATA_STRUCTURES_SHM_STRUCT_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_archive.h"
#include "shm_container.h"

namespace labstor::ipc {

/** Create a ShmHeader for the simple type */
template<typename T>
struct ShmSimpleHeader : public lipc::ShmBaseHeader {
  T obj_;

  template<typename ...Args>
  ShmSimpleHeader(Args&& ...args) : obj_(std::forward<Args>(args)...) {}
};

/**
 * MACROS used to simplify the vector namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME ShmStruct
#define TYPED_CLASS T
#define TYPED_HEADER ShmSimpleHeader<T>

/**
 * Used for storing a simple type (int, double, C-style struct, etc) in shared
 * memory.
 *
 * Called internally by manual_ptr, unique_ptr, and shared_ptr
 * */
template<typename T>
struct ShmStruct : public ShmContainer {
 public:
  /** Default constructor */
  ShmStruct() = default;

  /**
   * Constructs and stores a simple C type in shared-memory. E.g., a struct
   * or union. Complex structures should look at ShmContainer under
   * data_structures/data_structure.h
   * */
  template<typename ...Args>
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc, Args &&...args) {
    shm_init_allocator(alloc);
    shm_init_header(header, std::forward<Args>(args)...);
  }

  /** Weak move operator */
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc,
                          CLASS_NAME &other) {
    *header_ = *(other.header_);
  }

  /** Strong copy operator */
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc,
                            const CLASS_NAME &other) {
    *header_ = *(other.header_);
  }

  /** Serialize main */
  void shm_serialize_main() {}

  /** Deserialize main */
  void shm_deserialize_main() {}

  /** Destroy shared-memory data safely */
  void shm_destroy_main() {}

  /** Convert the pointer to a pointer */
  T* get() {
    return &header_->obj_;
  }

  /** Convert into a reference */
  T& get_ref() {
    return *get();
  }

  /** Convert the pointer to const pointer */
  T* get_const() const {
    return &header_->obj_;
  }

  /** Convert into a const reference */
  T& get_ref_const() const {
    return *get_const();
  }

  /** Convert into a pointer */
  T* operator->() {
    return get();
  }

  /** Convert into a reference */
  T& operator*() {
    return get_ref();
  }

  ////////////////////////////
  /// INHERITANCE BEGINS
  ////////////////////////////

 public:
  ////////////////////////////
  /// CLASS VARIABLES
  ////////////////////////////
  typedef TYPED_HEADER header_t;
  header_t *header_;

 public:
  ///////////////////////////
  /// SHM Init Constructors
  ////////////////////////////

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  explicit CLASS_NAME(Args&& ...args) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    shm_destroy(false);
    shm_init_main(typed_nullptr<TYPED_CLASS>(),
                  typed_nullptr<Allocator>(),
                  std::forward<Args>(args)...);
  }

  /** Constructor. Allocate header with specific allocator. */
  template<typename ...Args>
  void shm_init(lipc::Allocator *alloc, Args&& ...args) {
    shm_destroy(false);
    shm_init_main(typed_nullptr<TYPED_CLASS>(),
                  alloc,
                  std::forward<Args>(args)...);
  }

  /** Constructor. Initialize an already-allocated header. */
  template<typename ...Args>
  void shm_init(lipc::ShmHeader<CLASS_NAME> &header,
                lipc::Allocator *alloc, Args&& ...args) {
    shm_destroy(false);
    shm_init_main(&header, alloc, std::forward<Args>(args)...);
  }

  /**
   * Initialize a data structure's header.
   * A container will never re-set or re-allocate its header once it has
   * been set the first time.
   * */
  template<typename ...Args>
  void shm_init_header(lipc::ShmHeader<CLASS_NAME> *header,
                       Args&& ...args) {
    if (IsValid()) {
      // The container already has a valid header
      header_->SetBits(SHM_CONTAINER_DATA_VALID);
    } else if (header == nullptr) {
      // Allocate and initialize a new header
      header_ = alloc_->template
        AllocateConstructObjs<lipc::ShmHeader<CLASS_NAME>>(
        1, std::forward<Args>(args)...);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID |
          SHM_CONTAINER_HEADER_DESTRUCTABLE);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
          SHM_CONTAINER_DESTRUCTABLE);
    } else {
      // Initialize the input header
      Pointer header_ptr;
      header_ = header;
      Allocator::ConstructObj<lipc::ShmHeader<CLASS_NAME>>(
        *header_, header_ptr, std::forward<Args>(args)...);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
          SHM_CONTAINER_DESTRUCTABLE);
    }
  }

  ////////////////////////
  /// SHM Serialization
  ////////////////////////

  /** Serialize into a Pointer */
  void shm_serialize(TypedPointer<TYPED_CLASS> &ar) const {
    ar = alloc_->template
      Convert<Pointer>(header_);
    shm_serialize_main();
  }

  /** Serialize into an AtomicPointer */
  void shm_serialize(TypedAtomicPointer<TYPED_CLASS> &ar) const {
    ar = alloc_->template
      Convert<AtomicPointer>(header_);
    shm_serialize_main();
  }

  /** Override << operators */
  SHM_SERIALIZE_OPS((TYPED_CLASS))

  ////////////////////////
  /// SHM Deserialization
  ////////////////////////

  /** Deserialize object from a raw pointer */
  bool shm_deserialize(TypedPointer<TYPED_CLASS> &ar) {
    return shm_deserialize(
      LABSTOR_MEMORY_MANAGER->GetAllocator(ar.allocator_id_),
      ar.ToOffsetPointer()
    );
  }

  /** Deserialize object from allocator + offset */
  bool shm_deserialize(Allocator *alloc, OffsetPointer header_ptr) {
    if (header_ptr.IsNull()) { return false; }
    shm_deserialize(alloc,
                    LABSTOR_MEMORY_MANAGER->Convert<
                      TYPED_HEADER,
                      OffsetPointer>(header_ptr));
  }

  /** Deserialize object from allocator + header */
  bool shm_deserialize(Allocator *alloc,
                       TYPED_HEADER *header) {
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
    alloc_ = alloc;
    header_ = header;
    flags_.SetBits(SHM_CONTAINER_VALID);
    shm_deserialize_main();
    return true;
  }

  /** Constructor. Deserialize the object from the reference. */
  template<typename ...Args>
  void shm_init(lipc::Ref<TYPED_CLASS> &ref) {
    shm_destroy(false);
    shm_deserialize(ref.obj_.GetAllocator(), ref.obj_.header_);
  }

  /** Override >> operators */
  SHM_DESERIALIZE_OPS((TYPED_CLASS))

  ////////////////////////
  /// Destructors
  ////////////////////////

  /** Destructor */
  ~CLASS_NAME() {
    if (IsDestructable()) {
      shm_destroy(true);
    }
  }

  /** Shm Destructor */
  void shm_destroy(bool destroy_header = true) {
    if (!IsValid()) { return; }
    if (IsDataValid()) {
      shm_destroy_main();
    }
    UnsetDataValid();
    if (destroy_header &&
      header_->OrBits(SHM_CONTAINER_HEADER_DESTRUCTABLE)) {
      alloc_->FreePtr(header_);
      UnsetValid();
    }
  }

  ////////////////////////
  /// Move Constructors
  ////////////////////////

  /** Move constructor */
  CLASS_NAME(CLASS_NAME &&other) noexcept {
    shm_weak_move(typed_nullptr<TYPED_CLASS>(),
                  typed_nullptr<Allocator>(),
                  other);
  }

  /** Move assignment operator */
  CLASS_NAME& operator=(CLASS_NAME &&other) noexcept {
    if (this != &other) {
      shm_weak_move(typed_nullptr<TYPED_CLASS>(),
                    typed_nullptr<Allocator>(),
                    other);
    }
    return *this;
  }

  /** Move shm_init constructor */
  void shm_init_main(lipc::ShmHeader<CLASS_NAME> *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &&other) noexcept {
    shm_weak_move(typed_nullptr<TYPED_CLASS>(),
                  typed_nullptr<Allocator>(),
                  other);
  }

  /** Move operation */
  void shm_weak_move(lipc::ShmHeader<CLASS_NAME> *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &other) {
    if (other.IsNull()) { return; }
    header_->SetBits(SHM_CONTAINER_DATA_VALID);
    shm_weak_moveMain(header, alloc, other);
    if (!IsDestructable() || !other.IsDestructable()) {
      UnsetDestructable();
    }
    other.UnsetDataValid();
    other.shm_destroy(true);
  }

  ////////////////////////
  /// Copy Constructors
  ////////////////////////

  /** Copy constructor */
  CLASS_NAME(const CLASS_NAME &other) noexcept {
    shm_init(other);
  }

  /** Copy assignment constructor */
  CLASS_NAME& operator=(const CLASS_NAME &other) {
    if (this != &other) {
      shm_strong_copy(typed_nullptr<TYPED_CLASS>(),
                      typed_nullptr<Allocator>(),
                      other);
    }
    return *this;
  }

  /** Copy shm_init constructor */
  void shm_init_main(lipc::ShmHeader<CLASS_NAME> *header,
                     lipc::Allocator *alloc,
                     const CLASS_NAME &other) {
    shm_strong_copy(header, alloc, other);
  }

  /** Strong Copy operation */
  void shm_strong_copy(lipc::ShmHeader<CLASS_NAME> *header,
                       lipc::Allocator *alloc,
                       const CLASS_NAME &other) {
    if (other.IsNull()) { return; }
    header_->SetBits(SHM_CONTAINER_DATA_VALID);
    shm_strong_copy_main(header, alloc, other);
    if (!IsDestructable() || !other.IsDestructable()) {
      UnsetDestructable();
    }
    other.UnsetDataValid();
    other.shm_destroy(true);
  }

  /////////////////////
  /// Flag Operations
  /////////////////////

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
    return flags_.OrBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if header's data is valid */
  bool IsDataValid() const {
    return header_->OrBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Check if header's data is valid */
  void UnsetDataValid() const {
    header_->UnsetBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Check if container has a valid header */
  bool IsValid() const {
    return flags_.OrBits(SHM_CONTAINER_VALID);
  }

  /** Set container header invalid */
  void UnsetValid() {
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if null */
  bool IsNull() const {
    return !IsValid() || !IsDataValid();
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_DATA_STRUCTURES_SHM_STRUCT_H_
