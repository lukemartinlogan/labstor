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
 * The unique_pointer shared-memory header
 * */
template<typename T>
struct unique_ptr_header {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
 public:
  ShmArchive<T> obj_;
};

/**
 * MACROS to simplify the unordered_map namespace
 * */
#define CLASS_NAME unique_ptr
#define TYPED_CLASS unique_ptr<T>
#define TYPED_HEADER unique_ptr_header<T>

/**
 * Creates a unique instance of a shared-memory data structure
 * and deletes eventually.
 * */
template<typename T>
class unique_ptr : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
public:
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)

 private:
  typedef SHM_T_PTR_OR_ARCHIVE(T) T_PAr;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  bool owner_;
  T_PAr obj_;
  Pointer obj_ptr_;

 public:
  template<typename ...Args>
  explicit unique_ptr(Allocator *alloc, Args&& ...args) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_ << T(std::forward<Args>(args)...);
    } else {
      obj_ = alloc->AllocateConstructObjs<T>(1, obj_ptr_,
                                             std::forward<Args>(args)...);
    }
    owner_ = true;
  }

  T_Ref get() {
  }

  void shm_deserialize(ShmArchive<TYPED_CLASS> &ar) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_ << ar.obj_;
      owner_ = false;
    }
  }

  void operator=(unique_ptr<T> &&other) = delete;

  ~unique_ptr() {
    if (!owner_) { return; }
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj_.shm_destroy();
    } else {
      alloc_->Free();
    }
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
