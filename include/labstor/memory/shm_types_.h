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

/**
 * Indicates that a data structure supports shared-memory storage.
 * Must implement the interfaces in the below comments.
 * */
class ShmSerializeable {
  /**
   * Every ShmSerializeable data structure must be initialized using
   * shm_init. Constructors may wrap around these.
   *
   * This is not a virtual function, as each data structure
   * will have different parameters to this. Templates are used
   * to compensate for differences between data structures
   * */
  // virtual void shm_init(std::forward<Args>(args)...) = 0;

  /**
   * Every ShmSerializeable data structure must be destroyed using
   * shm_destroy. Destructors may wrap around this.
   * */
  // virtual void shm_destroy() = 0;
};

/**
 * Implements generic shm_serialize and deserialize methods to use
 * in subsequent data structures.
 * */
template<typename TYPED_HEADER>
class ShmSerializer : public ShmSerializeable {
 protected:
  Pointer header_ptr_;
  Allocator *alloc_;
  TYPED_HEADER *header_;

 public:
  /** Default constructor */
  ShmSerializer()
  : header_ptr_(kNullPointer), alloc_(nullptr), header_(nullptr) {};

  /** Serialize an object into a raw pointer */
  void shm_serialize(Pointer &header_ptr) const;

  /** Deserialize object from a raw pointer */
  void operator>>(Pointer &header_ptr) const {
    shm_serialize(header_ptr);
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr);

  /** Deserialize object from a raw pointer */
  void operator<<(const Pointer &header_ptr) {
    shm_deserialize(header_ptr);
  }

  /** Set to null */
  void SetNull() {
    header_ptr_.set_null();
  }

  /** Check if null */
  bool IsNull() {
    return header_ptr_.is_null();
  }

  /** Move all pointers to another */
  void WeakCopy(const ShmSerializer &other) {
    header_ptr_ = other.header_ptr_;
    alloc_ = other.alloc_;
    header_ = other.header_;
  }

  /** Move all pointers to another */
  void WeakMove(ShmSerializer &other) {
    if (this != &other) {
      WeakCopy(other);
      other.SetNull();
    }
  }

  /** Get the allocator for this pointer */
  Allocator* GetAllocator() {
    return alloc_;
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const;
};

#define SHM_SERIALIZER_TEMPLATE(TYPED_HEADER)\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::header_ptr_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::alloc_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::header_;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::shm_serialize;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::shm_deserialize;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::IsNull;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::SetNull;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::WeakCopy;\
using labstor::ipc::ShmSerializer<TYPED_HEADER>::WeakMove;

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
struct ShmPointer : public ShmSerializer<T> {
  SHM_SERIALIZER_TEMPLATE(T)

  /** Default constructor -- Does nothing */
  ShmPointer() = default;

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

  /** (De)serialize ShmPointer into ShmArchive<T> */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(T)

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
