//
// Created by lukemartinlogan on 1/15/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_REF_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_REF_H_

#include "labstor/constants/macros.h"
#include "shm_macros.h"
#include "shm_archive.h"

namespace labstor::ipc {

/**
 * A reference to a shared-memory object are a simple object
 * stored in shared-memory.
 * */
template<typename T>
struct Ref {
  typedef SHM_T_OR_PTR_T(T) T_Ptr;
  typedef SHM_ARCHIVE_OR_REF(T) T_Ar;
  T_Ptr obj_;

  /** Default constructor */
  Ref() = default;

  /**
   * Constructor. Either reference a SHM_ARCHIVE or a reference to a
   * data type. Deserializes the object if it's archiveable.
   * */
  explicit Ref(T_Ar other) {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other);
    } else {
      obj_ = &other;
    }
  }

  /** Copy constructor */
  Ref(const Ref &other) {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other);
    } else {
      obj_ = other.obj_;
    }
  }

  /** Move constructor */
  Ref(Ref &&other) noexcept {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      obj_.shm_deserialize(other);
    } else {
      obj_ = other.obj_;
    }
  }

  /** Copy assign operator */
  Ref& operator=(const Ref &other) {
    if (this != &other) {
      if constexpr(IS_SHM_ARCHIVEABLE(T)) {
        obj_.shm_deserialize(other.obj_.ar_);
      } else {
        obj_ = other.obj_;
      }
    }
    return *this;
  }

  /** Move assign operator */
  Ref& operator=(Ref &&other) noexcept {
    if (this != &other) {
      if constexpr(IS_SHM_ARCHIVEABLE(T)) {
        obj_.shm_deserialize(other.obj_);
      } else {
        obj_ = other.obj_;
      }
    }
    return *this;
  }

  /** Get reference to the internal data structure */
  T& get_ref() {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      return obj_;
    } else {
      return *obj_;
    }
  }

  /** Get a constant reference */
  const T& get_ref_const() const {
    if constexpr(IS_SHM_ARCHIVEABLE(T)) {
      return obj_;
    } else {
      return *obj_;
    }
  }

  /** Dereference operator */
  T& operator*() {
    return get_ref();
  }

  /** Constant dereference operator */
  const T& operator*() const {
    return get_ref_const();
  }

  /** Pointer operator */
  T* operator->() {
    return &get_ref();
  }

  /** Constant pointer operator */
  const T* operator->() const {
    return &get_ref_const();
  }
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_REF_H_
