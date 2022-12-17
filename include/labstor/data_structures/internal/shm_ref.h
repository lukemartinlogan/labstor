//
// Created by lukemartinlogan on 12/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "shm_ar.h"

namespace labstor::ipc {

/**
 * Stores a "shared-memory reference". I.e., the result of
 * constructing an object from an archive.
 * */
template<typename T>
class _shm_ref_shm {
 public:
  ShmArchive<T> &obj_ar_;
  mptr<T> obj_;

  /** Deserializes shared-memory data structure from an archive */
  explicit _shm_ref_shm(shm_ar<T> &obj_ar) : obj_ar_(obj_ar.internal_ref()) {
    obj_.shm_deserialize(obj_ar_);
  }

  /** Deserializes shared-memory data structure from an archive */
  explicit _shm_ref_shm(ShmArchive<T> &obj_ar) : obj_ar_(obj_ar) {
    obj_.shm_deserialize(obj_ar_);
  }

  /** Gets the data in a way that exists after this object is destroyed */
  T export_data() {
    return *obj_;
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

  /** Copy assignment operator */
  _shm_ref_shm& operator=(const T &obj) {
    (*obj_) = obj;
    obj_.shm_serialize(obj_ar_);
    return *this;
  }

  /** Move assignment operator */
  _shm_ref_shm& operator=(T &&obj) {
    (*obj_) = std::move(obj);
    obj_.shm_serialize(obj_ar_);
    return *this;
  }

  /** Move constructor */
  _shm_ref_shm(_shm_ref_shm &&other) noexcept
  : obj_(std::move(other.obj_)), obj_ar_(other.obj_ar_) {}

  /** Copy constructor */
  _shm_ref_shm(const _shm_ref_shm &other)
    : obj_(other.obj_), obj_ar_(other.obj_ar_) {}
};

/**
 * Stores a pointer to an object.
 * */
template<typename T>
class _shm_ref_noshm {
 public:
  T* obj_;

  /** Constructor. Stores the object reference */
  explicit _shm_ref_noshm(T &obj) : obj_(&obj) {}

  /** Constructor. Stores the object reference form archive. */
  explicit _shm_ref_noshm(shm_ar<T> &obj_ar) : obj_(&obj_ar.internal_ref()) {
  }

  /** Gets the data in a way that exists after this object is destroyed */
  T& export_data() {
    return get_ref();
  }

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

  /** Copy assignment operator */
  _shm_ref_noshm& operator=(const T &obj) {
    (*obj_) = obj;
    return *this;
  }

  /** Move assignment operator */
  _shm_ref_noshm& operator=(T &&obj) {
    (*obj_) = std::move(obj);
    return *this;
  }

  /** Move constructor */
  _shm_ref_noshm(_shm_ref_noshm &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  _shm_ref_noshm(const _shm_ref_noshm &other)
  : obj_(other.obj_) {}
};

/**
 * Whether or not to use _shm_ref_shm or _shm_ref_noshm
 * */
#define SHM_MAKE_T_OR_SHM_REF_T(T) \
  SHM_X_OR_Y(T, _shm_ref_shm<T>, _shm_ref_noshm<T>)

/**
 * If T represents a shared-memory object, store mptr<T>.
 * If T represents a non-shm object, store T*
 *
 * This object assumes that another data structure owns the object, nad
 * is not responsible for freeing it.
 * */
template<typename T>
class shm_ref : public ShmSmartPointer {
 public:
  typedef SHM_T_OR_REF_T(T) T_Ref;
  SHM_MAKE_T_OR_SHM_REF_T(T) obj_;

 public:
  /** Constructor */
  template<typename ...Args>
  explicit shm_ref(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}

  /** Destructor. Does nothing. */
  ~shm_ref() = default;

  /** Gets the data in a way that exists after this object is destroyed */
  T_Ref export_data() {
    return obj_.export_data();
  }

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

  /** Implicitly convert shm_ref into T& */
  operator T&() {
    return get_ref();
  }

  /** Copy assignment operator */
  shm_ref& operator=(const T &obj) {
    obj_ = obj;
    return *this;
  }

  /** Move assignment operator */
  shm_ref& operator=(T &&obj) {
    obj_ = std::move(obj);
    return *this;
  }

  /** Move constructor */
  shm_ref(shm_ref &&other) noexcept
  : obj_(std::move(other.obj_)) {}

  /** Copy constructor */
  shm_ref(const shm_ref &other)
    : obj_(other.obj_) {}
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_shm_ref_H_
