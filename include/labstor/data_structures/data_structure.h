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
    header_(nullptr), alloc_(alloc), mem_mngr_(LABSTOR_MEMORY_MANAGER) {}

  explicit ShmDataStructure(allocator_id_t alloc_id) :
    header_(nullptr), mem_mngr_(LABSTOR_MEMORY_MANAGER) {
    alloc_ = mem_mngr_->GetAllocator(alloc_id);
  }

  void shm_serialize(ShmArchive<void> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<void> &ar) {
    header_ptr_ = ar.header_ptr_;
    header_ = mem_mngr_->template
      Convert<TYPED_HEADER>(header_ptr_);
    alloc_ = mem_mngr_->GetAllocator(header_ptr_.allocator_id_);
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

template<typename T, typename T_Ar, typename ...Args>
void _construct(T_Ar &obj_ar, Args ...args) {
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj(args...);
    obj >> obj_ar;
  } else {
    new(&obj_ar) T(args...);
  }
}

template<typename T, typename T_Ar>
void _destruct(T_Ar &obj_ar) {
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj;
    obj << obj_ar;
    obj.shm_destroy();
  }
  obj_ar.~T_Ar();
}

}  // namespace labstor::ipc

#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
 public:\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::header_ptr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::mem_mngr_;\
  using labstor::ipc::ShmDataStructure<TYPED_CLASS, TYPED_HEADER>::alloc_;\
  explicit CLASS_NAME(labstor::ipc::ShmArchive<TYPED_CLASS> &ar) {\
    shm_deserialize(ar);\
  }


#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
