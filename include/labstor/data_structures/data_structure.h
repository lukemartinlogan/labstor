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

template<typename TYPED_CLASS>
class ShmDataStructure : public ShmSerializeable<TYPED_CLASS> {
 protected:
  Pointer header_ptr_;
  ShmHeader<TYPED_CLASS> *header_;
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

  void InitDataStructure(Pointer &header_ptr) {
    header_ = mem_mngr_->template
      Convert<ShmHeader<TYPED_CLASS>>(header_ptr_);
    header_ptr_ = header_ptr;
    alloc_ = mem_mngr_->GetAllocator(header_ptr.allocator_id_);
  }
};

template<typename T, typename T_Ar, typename ...Args>
void _construct(T_Ar &obj_ar, Args ...args) {
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj(args...);
    obj_ar << obj;
  } else {
    new (&obj_ar) T(args...);
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

#define SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS)\
 public:\
  using ShmDataStructure<TYPED_CLASS>::header_;\
  using ShmDataStructure<TYPED_CLASS>::header_ptr_;\
  using ShmDataStructure<TYPED_CLASS>::mem_mngr_;\
  using ShmDataStructure<TYPED_CLASS>::alloc_;\
  using ShmDataStructure<TYPED_CLASS>::InitDataStructure;\
  SHM_SERIALIZEABLE_TEMPLATE(CLASS_NAME, TYPED_CLASS)
}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
