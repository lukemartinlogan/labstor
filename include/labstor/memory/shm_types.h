//
// Created by lukemartinlogan on 12/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_TYPES_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_TYPES_H_

#include "memory_manager.h"
#include "shm_macros.h"
#include "shm_types_.h"

namespace labstor::ipc {

/**
 * SHMArchive Implementation
 * */

/** Get the process-independent pointer */
template<typename T>
Pointer& ShmArchive<T>::Get() {
  return header_ptr_;
}

/** Get the process-independent pointer */
template<typename T>
const Pointer& ShmArchive<T>::GetConst() {
  return header_ptr_;
}

/** Constructs and archives an object */
template<typename T>
template<typename ...Args>
ShmArchive<T>::ShmArchive(Args&& ...args) {
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj(std::forward<Args>(args)...);
    if constexpr(!IS_SHM_SMART_POINTER(T)) {
      obj.UnsetDestructable();
    }
    obj >> (*this);
  } else {
    ShmPointer<T>(std::forward<Args>(args)...) >> (*this);
  }
}

/** Creates a ShmArchive from a header pointer */
template<typename T>
ShmArchive<T>::ShmArchive(Pointer &ptr)
: header_ptr_(ptr) {
}

/** Creates a ShmArchive from a header pointer */
template<typename T>
ShmArchive<T>::ShmArchive(const Pointer &ptr)
: header_ptr_(ptr) {
}

/** Copies a ShmArchive into another */
template<typename T>
ShmArchive<T>::ShmArchive(const ShmArchive &other)
: header_ptr_(other.header_ptr_) {
}

/** Moves the data from one archive into another */
template<typename T>
ShmArchive<T>::ShmArchive(ShmArchive&& source) noexcept
: header_ptr_(source.header_ptr_) {
  source.header_ptr_.set_null();
}

/**
 * ShmPointer implementation
 * */

/**
 * Constructs and stores a simple C type in shared-memory. E.g., a struct
 * or union. Complex structures should look at ShmDataStructure under
 * data_structures/data_structure.h
 * */
template<typename T>
template<typename ...Args>
void ShmPointer<T>::shm_init(Allocator *alloc, Args&& ...args) {
  alloc_ = alloc;
  if (alloc_ == nullptr) {
    alloc_ = LABSTOR_MEMORY_MANAGER->GetDefaultAllocator();
  }
  header_ = alloc_->AllocateConstructObjs<T>(1, header_ptr_,
                                             std::forward<Args>(args)...);
}

/** Destroy the contents of the ShmPointer */
template<typename T>
void ShmPointer<T>::shm_destroy() {
  if (IsNull()) { return; }
  alloc_->Free(header_ptr_);
}

/**
 * Store the ShmPointer in shared memory
 * */
template<typename T>
void ShmPointer<T>::shm_serialize(ShmArchive<T> &ar) const {
  ar.header_ptr_ = header_ptr_;
}

/**
 * Retrieve the ShmPointer from shared memory
 * */
template<typename T>
void ShmPointer<T>::shm_deserialize(ShmArchive<T> &ar) {
  header_ptr_ = ar.header_ptr_;
  if (header_ptr_.is_null()) { return; }
  alloc_ = LABSTOR_MEMORY_MANAGER->GetAllocator(header_ptr_.allocator_id_);
  header_ = LABSTOR_MEMORY_MANAGER->Convert<T>(header_ptr_);
}

/** Set to null */
template<typename T>
void ShmPointer<T>::SetNull() {
  header_ptr_.set_null();
}

/** Check if null */
template<typename T>
bool ShmPointer<T>::IsNull() {
  return header_ptr_.is_null();
}

/** Get the allocator for this pointer */
template<typename T>
Allocator* ShmPointer<T>::GetAllocator() {
  return alloc_;
}

/** Convert the pointer to an object */
template<typename T>
T* ShmPointer<T>::get() {
  return header_;
}

/** Convert into a reference */
template<typename T>
T& ShmPointer<T>::get_ref() {
  return *get();
}

/** Convert the pointer to an object */
template<typename T>
T* ShmPointer<T>::get_const() const {
  return header_;
}

/** Convert into a reference */
template<typename T>
T& ShmPointer<T>::get_ref_const() const {
  return *get_const();
}

/** Convert into a pointer */
template<typename T>
T* ShmPointer<T>::operator->() {
  return get();
}

/** Convert into a reference */
template<typename T>
T& ShmPointer<T>::operator*() {
  return get_ref();
}

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_TYPES_H_
