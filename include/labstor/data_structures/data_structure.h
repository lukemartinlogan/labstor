//
// Created by lukemartinlogan on 11/2/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_

#include <labstor/memory/memory.h>
#include <labstor/memory/allocator/allocator.h>
#include <labstor/memory/memory_manager.h>
#include <labstor/constants/singleton_macros.h>

namespace labstor {

template<typename T>
class ShmDataStructure : public ShmSerializeable<T> {
 protected:
  memory::Pointer header_ptr_;
  ShmHeader<T> *header_;
  memory::Allocator *alloc_;
  LABSTOR_MEMORY_MANAGER_T mem_mngr_;

 public:
  ShmDataStructure(memory::Allocator *alloc) :
    alloc_(alloc), header_(nullptr) {
    mem_mngr_ = LABSTOR_MEMORY_MANAGER;
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_DATA_STRUCTURE_H_
