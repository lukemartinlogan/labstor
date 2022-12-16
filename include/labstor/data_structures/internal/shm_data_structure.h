//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include <labstor/constants/data_structure_singleton_macros.h>

#include "labstor/data_structures/internal/shm_macros.h"
#include "labstor/data_structures/internal/shm_archive.h"
#include "labstor/data_structures/internal/shm_pointer.h"
#include "labstor/data_structures/internal/shm_construct.h"

namespace labstor::ipc {

/**
 * The general base class of a shared-memory data structure
 * */
template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmDataStructure : public ShmSerializer<TYPED_HEADER> {
 public:
  SHM_SERIALIZER_TEMPLATE(TYPED_HEADER)
  typedef TYPED_HEADER header_t;

 protected:
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;  /**< Memory manager */
  bool destructable_;  /**< Whether or not to call shm_destroy in destructor */

 public:
  /** Default constructor */
  ShmDataStructure()
    : mem_mngr_(LABSTOR_MEMORY_MANAGER), destructable_(true) {
  }

  /** Copy constructor */
  ShmDataStructure(const ShmDataStructure &other) {
    WeakCopy(other);
    SetDestructable();
  }

  /** Initialize shared-memory data structure */
  void shm_init(Allocator *alloc) {
    if (alloc == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    } else {
      alloc_ = alloc;
    }
  }

  /** Copy only pointers */
  void WeakCopy(const ShmDataStructure &other) {
    mem_mngr_ = other.mem_mngr_;
    destructable_ = other.destructable_;
    ShmSerializer<TYPED_HEADER>::WeakCopy(other);
  }

  /** Move only pointers */
  void WeakMove(ShmDataStructure &other) {
    WeakCopy(other);
    other.SetNull();
  }

  /** Move constructor */
  ShmDataStructure(ShmDataStructure &&other) noexcept {
    WeakMove(other);
  }

  /** Move assignment operator */
  ShmDataStructure&
  operator=(ShmDataStructure &&other) noexcept {
    if (this != &other) {
      WeakMove(other);
    }
    return *this;
  }

  /** Sets this object as destructable */
  void SetDestructable() {
    destructable_ = true;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    destructable_ = false;
  }

  /** Serialize the data structure into a ShmArchive */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)
};

}  // namespace labstor::ipc

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_ptr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::mem_mngr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::alloc_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::destructable_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_serialize;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_deserialize;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::IsNull;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::SetDestructable;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::UnsetDestructable;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::WeakMove;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::WeakCopy;

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_
