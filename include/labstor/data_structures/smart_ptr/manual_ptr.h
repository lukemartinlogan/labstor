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
  SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T);

 public:
  /** Default constructor does nothing */
  manual_ptr() = default;

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    obj_.shm_init(std::forward<Args>(args)...);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Destructor. Does nothing -- must manually call shm_destroy */
  ~manual_ptr() = default;

  /** Copy constructor */
  manual_ptr(const manual_ptr &other) {
    WeakCopy(other);
  }

  /** Move constructor */
  manual_ptr(manual_ptr&& source) noexcept {
    WeakMove(source);
  }

  /** Copy assignment operator */
  manual_ptr<T>& operator=(const manual_ptr<T> &other) {
    if (this != &other) {
      WeakCopy(other);
    }
    return *this;
  }

  /** Move a manual_ptr */
  void WeakMove(manual_ptr &other) {
    obj_.WeakMove(other.obj_);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Copy a manual_ptr */
  void WeakCopy(const manual_ptr &other) {
    obj_.WeakCopy(other.obj_);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Loads an object from a ShmArchive<T> */
  template<typename ...Args>
  explicit manual_ptr(ShmArchive<T> &ar) {
    shm_deserialize(ar);
  }

  /** Loads an object from a ShmArchive<mptr> */
  explicit manual_ptr(ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize(ar);
  }

  /** Loads an object from a ShmArchive<uptr> */
  explicit manual_ptr(ShmArchive<uptr<T>> &ar) {
    shm_deserialize(ar);
  }

  /** (De)serialize the obj from a ShmArchive<T> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(T);

  /** (De)serialize the obj from a ShmArchive<mptr<T>> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(manual_ptr<T>);

  /** Deserialize the obj from a ShmArchive<uptr<T>> */
  SHM_DESERIALIZE_WRAPPER(uptr<T>);
};

template<typename T>
using mptr = manual_ptr<T>;

template<typename T, typename ...Args>
static mptr<T> make_mptr(Args&& ...args) {
  mptr<T> ptr;
  ptr.shm_init(std::forward<Args>(args)...);
  return ptr;
}

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
