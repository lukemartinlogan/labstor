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

  /** Serialize into a raw pointer */
  void shm_serialize(Pointer &header_ptr) const {
    obj_.shm_serialize(header_ptr);
  }

  /** Serialize into a raw pointer */
  void operator>>(Pointer &header_ptr) const {
    shm_serialize(header_ptr);
  }

  /** Deserialize from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr) {
    obj_.shm_deserialize(header_ptr);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Deserialize from a raw pointer */
  void operator<<(const Pointer &header_ptr) {
    shm_deserialize(header_ptr);
  }
};

/**
 * The general base class of a shared-memory data structure
 * */
template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmDataStructure : public ShmSerializer<TYPED_HEADER> {
 public:
  SHM_SERIALIZER_TEMPLATE(TYPED_HEADER)
  typedef TYPED_HEADER header_t;

 protected:
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;  /**< Memory manager */
  bool destructable_;  /**< Whether or not to call shm_destroy in destructor */

 public:
  /** Default constructor */
  ShmDataStructure()
  : mem_mngr_(LABSTOR_MEMORY_MANAGER), destructable_(true) {
  }

  /** Copy constructor */
  ShmDataStructure(const ShmDataStructure &other) {
    WeakCopy(other);
    SetDestructable();
  }

  /** Initialize shared-memory data structure */
  void shm_init(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /** Copy only pointers */
  void WeakCopy(const ShmDataStructure &other) {
    mem_mngr_ = other.mem_mngr_;
    destructable_ = other.destructable_;
    ShmSerializer<TYPED_HEADER>::WeakCopy(other);
  }

  /** Move only pointers */
  void WeakMove(ShmDataStructure &other) {
    WeakCopy(other);
    other.SetNull();
  }

  /** Move constructor */
  ShmDataStructure(ShmDataStructure &&other) noexcept {
    WeakMove(other);
  }

  /** Move assignment operator */
  ShmDataStructure&
  operator=(ShmDataStructure &&other) noexcept {
    if (this != &other) {
      WeakMove(other);
    }
    return *this;
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    destructable_ = true;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    destructable_ = false;
  }

  /** Serialize the data structure into a ShmArchive */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)
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
  using ShmDataStructurePointer<T>::shm_destroy;\
  using ShmDataStructurePointer<T>::shm_serialize;\
  using ShmDataStructurePointer<T>::shm_deserialize;

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
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::UnsetDestructable;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::WeakMove;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::WeakCopy;


#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
