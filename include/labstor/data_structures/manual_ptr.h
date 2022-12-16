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
  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  explicit manual_ptr(Args&& ...args) {
    shm_init(std::forward<Args>(args)...);
  }

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

  /** Copy constructor */
  manual_ptr(const manual_ptr &other) {
    obj_.WeakCopy(other);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** Move constructor */
  manual_ptr(manual_ptr&& source) noexcept {
    obj_.WeakMove(source.obj_);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.UnsetDestructable();
    }
  }

  /** (De)serialize the obj from a ShmArchive<T> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(T);

  /** (De)serialize the obj from a ShmArchive<mptr<T>> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(manual_ptr<T>);

  /** Deserialize the obj from a ShmArchive<uptr<T>> */
  SHM_DESERIALIZE_WRAPPER(uptr<T>);

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
