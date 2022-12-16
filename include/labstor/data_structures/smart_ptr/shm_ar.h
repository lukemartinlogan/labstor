//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_AR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_AR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "manual_ptr.h"

namespace labstor::ipc {

/**
 * Constructs a ShmArchive in-place
 * */
template<typename T>
class _shm_ar_shm : public ShmSmartPointer {
 public:
  ShmArchive<T> obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_shm(Args&& ...args)
  : obj_(make_shm_ar<T>(std::forward<Args>(args)...)) {
  }

  /** Destructor. Does nothing. */
  ~_shm_ar_shm() = default;

  /** Destroys memory allocated by this object */
  void shm_destroy() {
    mptr<T>(obj_).shm_destroy();
  }

  /** Returns the deserialized ShmArchive (just a few pointers) */
  T data() {
    return mptr<T>(obj_).get_ref();
  }

  /** Returns a reference to the internal object */
  ShmArchive<T>& internal_ref() {
    return obj_;
  }

  /** Move constructor */
  _shm_ar_shm(_shm_ar_shm &&other) noexcept
  : obj_(std::move(other.obj_)) {}
};

/**
 * Constructs an object in-place
 * */
template<typename T>
class _shm_ar_noshm {
 public:
  T obj_;

 public:
  /** Construct + store object */
  template<typename ...Args>
  explicit _shm_ar_noshm(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor. Does nothing. */
  ~_shm_ar_noshm() = default;

  /** Destroys memory allocated by this object */
  void shm_destroy() {
    Allocator::DestructObj<T>(obj_);
  }

  /** Gets the object */
  T& data() {
    return obj_;
  }

  /** Returns a reference to the internal object */
  T& internal_ref() {
    return obj_;
  }

  /** Move constructor */
  _shm_ar_noshm(_shm_ar_noshm &&other) noexcept
  : obj_(std::move(other.obj_)) {}
};

/**
 * Whether or not to use _shm_ar_shm or _shm_ar_noshm
 * */
#define SHM_MAKE_T_OR_ARCHIVE(T) \
  SHM_X_OR_Y(T, _shm_ar_shm<T>, _shm_ar_noshm<T>)

#define SHM_REF_OR_ARCHIVE(T)\
  SHM_X_OR_Y(T, ShmArchive<T>&, T&)

/**
 * Used for data structures which intend to store:
 * 1. An archive if the data type is SHM_SERIALIZEABLE
 * 2. The raw type if the data type is anything else
 *
 * E.g., used in unordered_map for storing collision entries.
 * E.g., used in a list for storing list entries.
 * */
template<typename T>
class shm_ar {
 public:
  typedef SHM_REF_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

  SHM_MAKE_T_OR_ARCHIVE(T) obj_;

  /** Construct + store object */
  template<typename ...Args>
  explicit shm_ar(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destruct. */
  ~shm_ar() {
    shm_destroy();
  }

  /** Destroys memory allocated by this object */
  void shm_destroy() {
    obj_.shm_destroy();
  }

  /** Returns the data represented by the archive */
  T_Ref data() {
    return obj_.data();
  }

  /** Returns a reference to the internal object */
  T_Ar& internal_ref() {
    return obj_.internal_ref();
  }

  /** Move constructor */
  shm_ar(shm_ar &&other) noexcept
  : obj_(std::move(other.obj_)) {}
};

} // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_AR_H_
