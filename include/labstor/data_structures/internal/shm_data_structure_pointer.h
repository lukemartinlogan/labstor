//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include "labstor/data_structures/internal/shm_macros.h"
#include <labstor/constants/data_structure_singleton_macros.h>

#include "labstor/data_structures/internal/shm_archive.h"
#include "labstor/data_structures/internal/shm_pointer.h"
#include "labstor/data_structures/internal/shm_construct.h"

namespace labstor::ipc {

/**
 * A base class used for creating shared-memory pointer management
 * classes (manual_ptr, unique_ptr, shared_ptr).
 * */
template<typename T>
class ShmDataStructurePointer : public ShmSmartPointer {
 public:
  typedef SHM_T_OR_SHM_PTR_T(T) T_Ptr;
  T_Ptr obj_;  /** T for complex object, ShmPointer<T> for C-style */

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

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_POINTER_H_
