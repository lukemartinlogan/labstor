//
// Created by lukemartinlogan on 12/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "unique_ptr.h"

namespace labstor::ipc {

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class shm_ref : public ShmDataStructurePointer<T> {
 public:
  typedef SHM_T_OR_PTR_T(T) T_Ptr;
  T_Ptr obj_;

 public:
  /** Stores a reference to a constructed object */
  explicit shm_ref(T &obj) {
    if constexpr(!IS_SHM_SERIALIZEABLE(T)) {
      obj_ = &obj;
    }
  }

  /** Deserializes shared-memory data structure an archive */
  explicit shm_ref(ShmArchive<T> obj_ar) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      shm_deserialize(obj_ar);
    }
  }

  /** Destructor. Does nothing -- must manually call shm_destroy */
  ~shm_ref() = default;

  /** Return a reference to the internal object */
  T_Ptr get() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return &obj_;
    } else {
      return obj_;
    }
  }

  /** Return a reference to the internal object */
  T& get_ref() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return obj_;
    } else {
      return *obj_;
    }
  }

  /** Return a reference to the internal object */
  T& operator*() {
    return get_ref();
  }

  /** Return a pointer to the internal object */
  T_Ptr operator->() {
    return get();
  }

  /** Deserialize the ptr from a ShmArchive<mptr> */
  void shm_deserialize(ShmArchive<T> &ar) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_ << ar;
      obj_.UnsetDestructable();
    }
  }

  /** Deserialize the ptr from a ShmArchive<mptr> */
  void operator<<(ShmArchive<T> &ar) {
    shm_deserialize(ar);
  }

};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
