//
// Created by lukemartinlogan on 11/2/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include "labstor/constants/singleton_macros.h"

namespace labstor::ipc {

template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmDataStructure : public ShmSerializeable {
 protected:
  Pointer header_ptr_;
  TYPED_HEADER *header_;
  Allocator *alloc_;
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;

 public:
  ShmDataStructure() :
    header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    alloc_ = mem_mngr_->GetDefaultAllocator();
  }

  explicit ShmDataStructure(Allocator *alloc) :
    header_(nullptr), alloc_(alloc), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    if (alloc_ == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    }
  }

  explicit ShmDataStructure(allocator_id_t alloc_id) :
    header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    alloc_ = mem_mngr_->GetAllocator(alloc_id);
    if (alloc_ == nullptr) {
      alloc_ = mem_mngr_->GetDefaultAllocator();
    }
  }

  Allocator *GetAllocator() {
    return alloc_;
  }

  void operator>>(ShmArchive<TYPED_CLASS> &ar) {
    shm_serialize(ar);
  }

  void operator<<(ShmArchive<TYPED_CLASS> &ar) {
    shm_deserialize(ar);
  }

  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<TYPED_CLASS> &ar) {
    header_ptr_ = ar.header_ptr_;
    header_ = mem_mngr_->template
      Convert<TYPED_HEADER>(header_ptr_);
    alloc_ = mem_mngr_->GetAllocator(header_ptr_.allocator_id_);
  }
};

}  // namespace labstor::ipc

#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_ptr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::mem_mngr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::alloc_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_serialize;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::shm_deserialize;\
  explicit CLASS_NAME(labstor::ipc::ShmArchive<TYPED_CLASS> &ar) {\
    shm_deserialize(ar);\
  }


#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
