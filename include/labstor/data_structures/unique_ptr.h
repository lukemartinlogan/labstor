//
// Created by lukemartinlogan on 11/9/22.
//

/**
 * Create a data structure in either shared or regular memory.
 * Destroy the data structure when the unique pointer is deconstructed
 * */

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

/**
 * MACROS to simplify the unique_ptr namespace
 * */
#define CLASS_NAME unique_ptr
#define TYPED_CLASS unique_ptr<T>

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class unique_ptr : public ShmDataStructurePointer<T> {
 public:
  SHM_DATA_STRUCTURE_POINTER_TEMPLATE(T);

 public:
  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  explicit unique_ptr(Args&& ...args) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Destroys all allocated memory */
  ~unique_ptr() {
    shm_destroy();
  }

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    obj_.shm_init(std::forward<Args>(args)...);
  }

  /** Disable the copy constructor */
  unique_ptr(const unique_ptr &other) = delete;

  /** Move constructor */
  unique_ptr(unique_ptr&& source) noexcept {
    obj_.WeakMove(source.obj_);
  }

  /** Serialize into a ShmArchive<unique_ptr> */
  SHM_SERIALIZE_WRAPPER(unique_ptr)

  /** Disables the assignment operator */
  void operator=(unique_ptr<T> &&other) = delete;
};

template<typename T>
using uptr = unique_ptr<T>;

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

namespace std {

/** Hash function for unique_ptr */
template<typename T>
struct hash<labstor::ipc::unique_ptr<T>> {
  size_t operator()(const labstor::ipc::unique_ptr<T> &obj) const {
    return std::hash<T>{}(obj.get_ref_const());
  }
};

}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
