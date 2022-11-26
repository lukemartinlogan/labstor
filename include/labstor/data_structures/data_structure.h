//
// Created by lukemartinlogan on 11/2/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_

#include <labstor/memory/memory.h>
#include <labstor/memory/allocator/allocator.h>
#include <labstor/memory/memory_manager.h>
#include <labstor/constants/singleton_macros.h>

namespace labstor::ipc {

template<typename T>
class ShmDataStructure : public ShmSerializeable<T> {
 protected:
  Pointer header_ptr_;
  ShmHeader<T> *header_;
  Allocator *alloc_;
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;

 public:
  ShmDataStructure() :
    header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    alloc_ = mem_mngr_->GetDefaultAllocator();
  }

  explicit ShmDataStructure(Allocator *alloc) :
    header_(nullptr), alloc_(alloc), mem_mngr_(LABSTOR_MEMORY_MANAGER) {}

  explicit ShmDataStructure(allocator_id_t alloc_id) :
    header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    alloc_ = mem_mngr_->GetAllocator(alloc_id);
  }
};

#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS)\
 public:\
  using ShmDataStructure<TYPED_CLASS>::header_;\
  using ShmDataStructure<TYPED_CLASS>::header_ptr_;\
  using ShmDataStructure<TYPED_CLASS>::mem_mngr_;\
  using ShmDataStructure<TYPED_CLASS>::alloc_;\
  SHM_SERIALIZEABLE_TEMPLATE(CLASS_NAME, TYPED_CLASS)
}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
