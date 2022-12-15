//
// Created by lukemartinlogan on 12/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY__SHM_TYPES_H
#define LABSTOR_INCLUDE_LABSTOR_MEMORY__SHM_TYPES_H

#include "shm_macros.h"

namespace labstor::ipc {

/** Forward declaration of pointer */
class Pointer;


/** Forward declaration of allocator */
class Allocator;

/** Forward  */

/**
 * Indicates that a data structure supports shared-memory storage by
 * implementing the interfaces shown in the comments
 * */
class ShmSerializeable {
 public:
  // virtual void shm_init(std::forward<Args>(args)...) = 0;
  // virtual void shm_destroy() = 0;
  // virtual void shm_serialize(ShmArchive ar) = 0;
  // virtual void shm_deserialize(ShmArchive ar) = 0;
  // void operator>>(ShmArchive ar);
  // void operator<<(ShmArchive r);
};

/**
 * Indicates a data structure represents a memory paradigm for Shm.
 * Needed because these data structures automatically determine
 * how to set
 * */
class ShmSmartPointer : public ShmSerializeable {};

/**
 * A wrapper around a process-independent pointer for storing
 * a single complex shared-memory data structure
 * */
template<typename T>
struct ShmArchive {
 public:
  Pointer header_ptr_;

  /** Default constructor */
  ShmArchive() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get();

  /** Get the process-independent pointer */
  inline const Pointer& GetConst();

  /** Constructs and archives a shm-serializeable object */
  template<typename ...Args>
  explicit ShmArchive(Args&& ...args);

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(Pointer &ptr);

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(const Pointer &ptr);

  /** Copies a ShmArchive into another */
  ShmArchive(const ShmArchive &other);

  /** Moves the data from one archive into another */
  ShmArchive(ShmArchive&& source) noexcept;
};

/**
 * Used for storing a simple type (C-style struct, etc) in shared
 * memory semantically.
 *
 * Called internally by ShmArchive.
 * Called internally by manual_ptr, unique_ptr, and shared_ptr
 * */
template<typename T>
struct ShmPointer {
  Pointer header_ptr_;
  Allocator *alloc_;
  T *header_;

  /** Default constructor -- Does nothing */
  ShmPointer() = default;

  /** Construct pointer in-place (explicit allocator) */
  template<typename ...Args>
  explicit ShmPointer(Allocator *alloc, Args&& ...args) {
    shm_init(alloc, std::forward<Args>(args)...);
  }

  /** Construct pointer in-place (find allocator) */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    shm_init(reinterpret_cast<Allocator*>(NULL),
             std::forward<Args>(args)...);
  }

  /**
   * Constructs and stores a simple C type in shared-memory. E.g., a struct
   * or union. Complex structures should look at ShmDataStructure under
   * data_structures/data_structure.h
   * */
  template<typename ...Args>
  void shm_init(Allocator *alloc, Args&& ...args);

  /** Destroy the contents of the ShmPointer */
  void shm_destroy();

  /** Set to null */
  void SetNull();

  /** Check if null */
  bool IsNull();

  /** Move all pointers to another */
  void WeakCopy(ShmPointer &other) {
    header_ptr_ = other.header_ptr_;
    alloc_ = other.alloc_;
    header_ = other.header_;
  }

  /** Move all pointers to another */
  void WeakMove(ShmPointer &other) {
    if (this != &other) {
      WeakCopy(other);
      other.SetNull();
    }
  }

  /** Serialize ShmPointer into archive */
  void shm_serialize(ShmArchive<T> &ar) const;

  /** Deserialize ShmPointer from archive */
  void shm_deserialize(ShmArchive<T> &ar);

  /** Serialize (>>) */
  void operator>>(ShmArchive<T> &ar) const {
    shm_serialize(ar);
  }

  /** Deserialize (<<) */
  void operator<<(ShmArchive<T> &ar) {
    shm_deserialize(ar);
  }

  /** Get the allocator for this pointer */
  Allocator* GetAllocator();

  /** Convert the pointer to a pointer */
  T* get();

  /** Convert into a reference */
  T& get_ref();

  /** Convert the pointer to const pointer */
  T* get_const() const;

  /** Convert into a const reference */
  T& get_ref_const() const;

  /** Convert into a pointer */
  T* operator->();

  /** Convert into a reference */
  T& operator*();
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY__SHM_TYPES_H
