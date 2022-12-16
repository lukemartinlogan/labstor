//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"

namespace labstor::ipc {

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
  void shm_serialize(Pointer &header_ptr) const {
    header_ptr = header_ptr_;
  }

  /** Deserialize object from a raw pointer */
  void operator>>(Pointer &header_ptr) const {
    shm_serialize(header_ptr);
  }

  /** Deserialize object from a raw pointer */
  void shm_deserialize(const Pointer &header_ptr) {
    header_ptr_ = header_ptr;
    if (header_ptr_.is_null()) { return; }
    alloc_ = LABSTOR_MEMORY_MANAGER->GetAllocator(header_ptr_.allocator_id_);
    header_ = LABSTOR_MEMORY_MANAGER->Convert<TYPED_HEADER>(header_ptr_);
  }

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
  void WeakMove(ShmSerializer &other) noexcept {
    WeakCopy(other);
    other.SetNull();
  }

  /** Move constructor */
  ShmSerializer(ShmSerializer &&other) noexcept {
    WeakMove(other);
  }

  /** Move assignment operator */
  ShmSerializer&
  operator=(ShmSerializer &&other) noexcept {
    if (this != &other) {
      WeakMove(other);
    }
    return *this;
  }

  /** Get the allocator for this pointer */
  Allocator* GetAllocator() {
    return alloc_;
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const {
    return alloc_->GetId();
  }
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

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_SERIALIZE_H_
