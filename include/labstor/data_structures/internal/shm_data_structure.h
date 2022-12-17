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

  /** Copy constructor */
  // void StrongCopy(const ShmDataStructure &other) {}

  /** Sets this object as destructable */
  void SetDestructable() {
    destructable_ = true;
  }

  /** Sets this object as nondestructable */
  void UnsetDestructable() {
    destructable_ = false;
  }

  /** Serialize / deserialize this object... */
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)
};

}  // namespace labstor::ipc

/**
 * Namespace simplification for a SHM data structure
 * */
#define SHM_DATA_STRUCTURE_USING_NS\
  using labstor::ipc::ShmDataStructure< \
    TYPE_UNWRAP(TYPED_CLASS), TYPE_UNWRAP(TYPED_HEADER)>

#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
  SHM_DATA_STRUCTURE_USING_NS::header_;\
  SHM_DATA_STRUCTURE_USING_NS::header_ptr_;\
  SHM_DATA_STRUCTURE_USING_NS::mem_mngr_;\
  SHM_DATA_STRUCTURE_USING_NS::alloc_;\
  SHM_DATA_STRUCTURE_USING_NS::destructable_;\
  SHM_DATA_STRUCTURE_USING_NS::shm_serialize;\
  SHM_DATA_STRUCTURE_USING_NS::shm_deserialize;\
  SHM_DATA_STRUCTURE_USING_NS::IsNull;\
  SHM_DATA_STRUCTURE_USING_NS::SetDestructable;\
  SHM_DATA_STRUCTURE_USING_NS::UnsetDestructable;\
  SHM_DATA_STRUCTURE_USING_NS::WeakMove;\
  SHM_DATA_STRUCTURE_USING_NS::WeakCopy;\
  SHM_INHERIT_MOVE_OPERATORS(CLASS_NAME)\
  SHM_INHERIT_COPY_OPERATORS(CLASS_NAME)\
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)

#define BASIC_SHM_DATA_STRUCTURE_TEMPLATE\
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME,\
    TYPE_WRAP(TYPED_CLASS), TYPE_WRAP(TYPED_HEADER))

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_
