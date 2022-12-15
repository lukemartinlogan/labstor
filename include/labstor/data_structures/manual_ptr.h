//
// Created by lukemartinlogan on 12/14/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"
#include "unique_ptr.h"

namespace labstor::ipc {

/**
 * MACROS to simplify the ptr namespace
 * */
#define CLASS_NAME manual_ptr
#define TYPED_CLASS manual_ptr<T>

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class manual_ptr : public ShmDataStructurePointer<T> {
 public:
  typedef SHM_T_OR_PTR_T(T) T_Ptr;
  T_Ptr obj_;

 public:

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  explicit manual_ptr(Args&& ...args) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      Allocator::RawConstructObj(obj_, std::forward<Args>(args)...);
      obj_.UnsetDestructable();
    } else {
      obj_ = new T(std::forward<Args>(args)...);
    }
  }

  /** Destructor. Does nothing -- must manually call shm_destroy */
  ~manual_ptr() = default;

  /** Loads an object from a ShmArchive<T> */
  template<typename ...Args>
  explicit manual_ptr(ShmArchive<T> ar) {
    shm_deserialize(ar);
  }

  /** Loads an object from a ShmArchive<mptr> */
  explicit manual_ptr(ShmArchive<TYPED_CLASS> ar) {
    shm_deserialize(ar);
  }

  /** Loads an object from a ShmArchive<uptr> */
  explicit manual_ptr(ShmArchive<uptr<T>> ar) {
    shm_deserialize(ar);
  }

  /** Copy constructor */
  manual_ptr(const manual_ptr &other) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.WeakCopy(other);
      obj_.UnsetDestructable();
    } else {
      obj_ = other.obj_;
    }
  }

  /** Move constructor */
  manual_ptr(manual_ptr&& source) noexcept {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.WeakMove(source);
      obj_.UnsetDestructable();
    } else {
      obj_ = source.obj_;
      source.obj_ = nullptr;
    }
  }

  /** Destroy memory */
  void shm_destroy() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.SetDestructable();
    } else {
      delete obj_;
    }
  }

  /** Serialize the ptr into a ShmArchive<mptr> */
  void operator>>(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize(ar);
  }

  /** Serialize the ptr into a ShmArchive<mptr> */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      auto cast = ShmArchive<T>(ar.Get());
      obj_ >> cast;
    }
  }

  /** Deserialize the ptr from a ShmArchive<T> */
  void operator<<(ShmArchive<T> &ar) {
    shm_deserialize(ar);
  }

  /** Deserialize the ptr from a ShmArchive<mptr> */
  void shm_deserialize(ShmArchive<T> &ar) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_ << ar;
      obj_.UnsetDestructable();
    }
  }

  /** Deserialize the ptr from a ShmArchive<mptr> */
  void operator<<(ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize(ar);
  }

  /** Deserialize the ptr from a ShmArchive<mptr> */
  void shm_deserialize(ShmArchive<TYPED_CLASS> &ar) {
    auto cast = ShmArchive<T>(ar.GetConst());
    shm_deserialize(cast);
  }

  /** Deserialize the ptr from a ShmArchive<uptr> */
  void operator<<(ShmArchive<uptr<T>> &ar) {
    shm_deserialize(ar);
  }

  /** Deserialize the ptr from a ShmArchive<uptr> */
  void shm_deserialize(ShmArchive<uptr<T>> &ar) {
    auto cast = ShmArchive<T>(ar.GetConst());
    shm_deserialize(cast);
  }

  /** Disables the assignment operator */
  void operator=(manual_ptr<T> &&other) = delete;
};

template<typename T>
using mptr = manual_ptr<T>;

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

namespace std {

/** Hash function for ptr */
template<typename T>
struct hash<labstor::ipc::manual_ptr<T>> {
  size_t operator()(const labstor::ipc::manual_ptr<T> &obj) const {
    return std::hash<T>{}(obj.get_ref_const());
  }
};

}  // namespace std

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PTR_H_
