//
// Created by lukemartinlogan on 12/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

/**
 * Stores a "shared-memory reference". I.e., the result of
 * constructing an object from an archive.
 * */
template<typename T>
class _shm_ref_shm {
 public:
  mptr<T> obj_;

  /** Deserializes shared-memory data structure an archive */
  explicit _shm_ref_shm(ShmArchive<T> &obj_ar) {
    obj_.shm_deserialize(obj_ar);
  }

  /** Return a pointer to the internal object */
  T* get() {
    return obj_.get();
  }

  /** Return a reference to the internal object */
  T& get_ref() {
    return obj_.get_ref();
  }

  /** Return a reference to the internal object */
  T& operator*() {
    return get_ref();
  }

  /** Return a pointer to the internal object */
  T* operator->() {
    return get();
  }
};

/**
 * Stores a pointer to an object.
 * */
template<typename T>
class _shm_ref_noshm {
 public:
  T* obj_;

  /** Stores the object reference */
  explicit _shm_ref_noshm(T &obj) : obj_(&obj) {}

  /** Return a pointer to the internal object */
  T* get() {
    return obj_;
  }

  /** Return a reference to the internal object */
  T& get_ref() {
    return *obj_;
  }

  /** Return a reference to the internal object */
  T& operator*() {
    return get_ref();
  }

  /** Return a pointer to the internal object */
  T* operator->() {
    return get();
  }
};

/**
 * Whether or not to use _shm_ref_shm or _shm_ref_noshm
 * */
#define SHM_MAKE_T_OR_REF_T(T) \
  SHM_X_OR_Y(T, _shm_ref_shm<T>, _shm_ref_noshm<T>)

/**
 * If T represents a shared-memory object, store T.
 * If T represents a non-shm object, store T*
 * */
template<typename T>
class shm_ref : public ShmSmartPointer {
 public:
  SHM_MAKE_T_OR_REF_T(T) obj_;

 public:
  /** Constructor */
  template<typename ...Args>
  explicit shm_ref(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor. Does nothing. */
  ~shm_ref() = default;

  /** Return a reference to the internal object */
  T* get() {
    return obj_.get();
  }

  /** Return a reference to the internal object */
  T& get_ref() {
    return obj_.get_ref();
  }

  /** Return a reference to the internal object */
  T& operator*() {
    return get_ref();
  }

  /** Return a pointer to the internal object */
  T* operator->() {
    return get();
  }
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
