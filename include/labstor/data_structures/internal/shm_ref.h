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
 * A reference to a shared-memory object
 * */
template<typename T>
struct _RefShm {
  T obj_;

  /** Default constructor */
  _RefShm() = default;

  /** Destructor */
  ~_RefShm() {
    obj_.UnsetDestructable();
  }

  /** Constructor. */
  explicit _RefShm(TypedPointer<T> other) {
    obj_.shm_deserialize(other);
  }

  /** Copy constructor */
  _RefShm(const _RefShm &other) {
    obj_.shm_deserialize(other.obj_);
  }

  /** Move constructor */
  _RefShm(_RefShm &&other) noexcept {
    obj_.shm_deserialize(other.obj_);
  }

  /** Copy assign operator */
  _RefShm& operator=(const _RefShm &other) {
    if (this != &other) {
      obj_.shm_deserialize(other.obj_);
    }
    return *this;
  }

  /** Move assign operator */
  _RefShm& operator=(_RefShm &&other) noexcept {
    if (this != &other) {
      obj_.shm_deserialize(other.obj_);
    }
    return *this;
  }

  /** Get reference to the internal data structure */
  T& get_ref() {
    return obj_;
  }

  /** Get a constant reference */
  const T& get_ref_const() const {
    return obj_;
  }
};

/**
 * A reference to a POD type stored in shared memory.
 * */
template<typename T>
struct _RefNoShm {
  T *obj_;

  /** Default constructor */
  _RefNoShm() = default;

  /** Constructor. */
  explicit _RefNoShm(T &other) {
    obj_ = &other;
  }

  /** Copy constructor */
  _RefNoShm(const _RefNoShm &other) {
    obj_ = other.obj_;
  }

  /** Move constructor */
  _RefNoShm(_RefNoShm &&other) noexcept {
    obj_ = other.obj_;
  }

  /** Copy assign operator */
  _RefNoShm& operator=(const _RefNoShm &other) {
    if (this != &other) {
      obj_ = other.obj_;
    }
    return *this;
  }

  /** Move assign operator */
  _RefNoShm& operator=(_RefNoShm &&other) noexcept {
    if (this != &other) {
      obj_ = other.obj_;
    }
    return *this;
  }

  /** Get reference to the internal data structure */
  T& get_ref() {
    return *obj_;
  }

  /** Get a constant reference */
  const T& get_ref_const() const {
    return *obj_;
  }
};

/** Determine whether Ref stores _RefShm or _RefNoShm */
#define CHOOSE_SHM_REF_TYPE(T) SHM_X_OR_Y(T, _RefShm<T>, _RefNoShm<T>)

/**
 * A reference to a shared-memory object or a simple object
 * stored in shared-memory.
 * */
template<typename T>
struct Ref {
  typedef CHOOSE_SHM_REF_TYPE(T) T_Ref;
  T_Ref obj_;

  /** Constructor. */
  template<typename ...Args>
  explicit Ref(Args&& ...args) : obj_(std::forward<Args>(args)...) {}

  /** Default constructor */
  Ref() = default;

  /** Copy Constructor */
  Ref(const Ref &other) : obj_(other.obj_) {}

  /** Move Constructor */
  Ref(Ref &&other) noexcept : obj_(std::move(other.obj_)) {}

  /** Copy assign operator */
  Ref& operator=(const Ref &other) {
    obj_ = other.obj_;
    return *this;
  }

  /** Move assign operator */
  Ref& operator=(Ref &&other) noexcept {
    obj_ = std::move(other.obj_);
    return *this;
  }

  /** Get reference to the internal data structure */
  T& get_ref() {
    return obj_.get_ref();
  }

  /** Get a constant reference */
  const T& get_ref_const() const {
    return obj_.get_ref_const();
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
