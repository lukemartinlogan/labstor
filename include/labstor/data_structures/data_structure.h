//
// Created by lukemartinlogan on 11/2/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include "labstor/memory/shm_types.h"
#include "labstor/memory/shm_macros.h"
#include <labstor/constants/data_structure_singleton_macros.h>
#include <labstor/memory/shm_types.h>

namespace labstor::ipc {

/**
 * A base class used for creating shared-memory pointer management
 * classes (manual_ptr, unique_ptr, shared_ptr).
 * */
template<typename T>
class ShmDataStructurePointer : public ShmSmartPointer {
 public:
  typedef SHM_T_OR_SHM_PTR_T(T) T_Ptr;
  T_Ptr obj_;

 public:
  /** Sets this pointer to NULL */
  void SetNull() {
    obj_.SetNull();
  }

  /** Checks if this pointer is null */
  bool IsNull() {
    return obj_.IsNull();
  }

  /** Gets a pointer to the internal object */
  T* get() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return &obj_;
    } else {
      return obj_.get();
    }
  }

  /** Gets a pointer to the internal object */
  const T* get_const() const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return &obj_;
    } else {
      return obj_.get_const();
    }
  }

  /** Gets a reference to the internal object */
  T& get_ref() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return obj_;
    } else {
      return obj_.get_ref();
    }
  }

  /** Gets a reference to the internal object */
  const T& get_ref_const() const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return obj_;
    } else {
      return obj_.get_ref_const();
    }
  }

  /** Gets a reference to the internal object using * */
  T& operator*() {
    return get_ref();
  }

  /** Gets a pointer to the internal object */
  T* operator->() {
    return get();
  }

  /** Destroy the data allocated by this pointer */
  void shm_destroy() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.SetDestructable();
    } else {
      obj_.shm_destroy();
    }
  }
};

/**
 * The general base class of a shared-memory data structure
 * */
template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmDataStructure : public ShmSerializeable {
 public:
  typedef TYPED_HEADER header_t;

 protected:
  Pointer header_ptr_;    /**< Independent pointer of typed header */
  TYPED_HEADER *header_;  /**< The typed shm header */
  Allocator *alloc_;      /**< The allocator */
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;  /**< Memory manager */
  bool destructable_;  /**< Whether or not to call shm_destroy in destructor */

 public:
  /** Default constructor */
  ShmDataStructure()
  : header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER),
  alloc_(nullptr), destructable_(true) {
  }

  /** Copy constructor */
  ShmDataStructure(const ShmDataStructure &other) {
    destructable_ = true;
  }

  /** Initialize shared-memory data structure */
  void shm_init(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /** Only copy pointers, not entire data structures */
  void WeakCopy(const TYPED_CLASS &other) {
    header_ptr_ = other.header_ptr_;
    header_ = other.header_;
    alloc_ = other.alloc_;
  }

  /** Only copy pointers, not entire data structures */
  void WeakMove(TYPED_CLASS &other) {
    WeakCopy(other);
    other.Nullify();
  }

  /** Nullifies data structure (for move) */
  void Nullify() {
    header_ptr_.set_null();
  }

  /** Checks if this data structure is null */
  bool IsNull() {
    return header_ptr_.is_null();
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    destructable_ = true;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    destructable_ = false;
  }

  /** Get the shared-memory allocator */
  Allocator* GetAllocator() {
    return alloc_;
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const {
    return alloc_->GetId();
  }

  /** Serialize the data structure into a ShmArchive */
  void operator>>(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize(ar);
  }

  /** Deserialize the data structure from a ShmArchive */
  void operator<<(const ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize(ar);
  }

  /** Serialize the data structure into a ShmArchive */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    ar.header_ptr_ = header_ptr_;
  }

  /** Deserialize the data structure from a ShmArchive */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    header_ptr_ = ar.header_ptr_;
    if (ar.header_ptr_.is_null()) { return; }
    header_ = mem_mngr_->template
      Convert<TYPED_HEADER>(header_ptr_);
    alloc_ = mem_mngr_->GetAllocator(header_ptr_.allocator_id_);
  }
};

}  // namespace labstor::ipc

/**
 * Namespace simplification for a SHM data structure pointer
 * */
#define SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T) \
  using ShmDataStructurePointer<T>::obj_;\
  using ShmDataStructurePointer<T>::get;\
  using ShmDataStructurePointer<T>::get_ref;\
  using ShmDataStructurePointer<T>::get_const;\
  using ShmDataStructurePointer<T>::get_ref_const;\
  using ShmDataStructurePointer<T>::SetNull;\
  using ShmDataStructurePointer<T>::IsNull;\
  using ShmDataStructurePointer<T>::shm_destroy;

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_ptr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::mem_mngr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::alloc_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::destructable_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_serialize;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_deserialize;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::IsNull;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::SetDestructable;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::UnsetDestructable;


#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
