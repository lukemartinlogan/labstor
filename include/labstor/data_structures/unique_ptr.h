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
  T_Ar obj_;

  template<typename ...Args>
  unique_ptr_header(Args&& ...args)
  : obj_(std::forward<Args>(args)...) {}
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
  typedef SHM_T_OR_REF_T(T) T_Ref;
  bool owner_;

 public:

  /** Allocates + constructs an object in shared memory */
  template<typename ...Args>
  explicit unique_ptr(Allocator *alloc, Args&& ...args)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Gets a reference to the internal object */
  T_Ref get() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return T(header_->obj_);
    } else {
      return header_->obj_;
    }
  }

  /** Gets a reference to the internal object */
  T_Ref get_const() const {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      return T(header_->obj_);
    } else {
      return header_->obj_;
    }
  }

  /** Gets a reference to the internal object using * */
  T_Ref operator*() {
    return get();
  }

  /** Disable the copy constructure */
  unique_ptr(const unique_ptr &other) = delete;

  /** Move constructor */
  unique_ptr(unique_ptr&& source) {
    header_ptr_ = source.header_ptr_;
    header_ = source.header_;
    owner_ = source.owner_;
    source.header_ptr_.set_null();
    source.owner_ = false;
  }

  /** Initializes shared memory header */
  template<typename ...Args>
  void shm_init(Args ...args) {
    header_ = alloc_->template
      AllocateConstructObjs<TYPED_HEADER>(1, header_ptr_,
                                          std::forward<Args>(args)...);
    owner_ = true;
  }

  /** Destroys shared memory allocated by data structure */
  void shm_destroy() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T(header_->obj_).shm_destroy();
    }
    alloc_->Free(header_ptr_);
  }

  /** Deserializes a unique_ptr from shared memory */
  void shm_deserialize(ShmArchive<TYPED_CLASS> &ar) {
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_deserialize(ar);
    owner_ = false;
  }

  /** Disables the assignment operator */
  void operator=(unique_ptr<T> &&other) = delete;

  /** Destroys all allocated memory */
  ~unique_ptr() {
    if (!owner_) { return; }
    shm_destroy();
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

namespace std {

/** Hash function for unique_ptr */
template<typename T>
struct hash<labstor::ipc::unique_ptr<T>> {
  size_t operator()(const labstor::ipc::unique_ptr<T> &obj) const {
    return std::hash<T>{}(obj.get_const());
  }
};

}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
