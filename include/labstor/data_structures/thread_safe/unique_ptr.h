//
// Created by lukemartinlogan on 11/9/22.
//

/**
 * Create a data structure in either shared or regular memory.
 * Destroy the data structure when the unique pointer is deconstructed
 * */

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

template<typename T>
class unique_ptr {
 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  T_Ar *obj_;

 public:
  template<typename ...Args>
  unique_ptr(Args&& ...args) {
  }

  T_Ref get() {
  }

  void operator=(unique_ptr<T> &&other) = delete;

  ~unique_ptr() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    }
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
