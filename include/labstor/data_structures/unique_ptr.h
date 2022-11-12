//
// Created by lukemartinlogan on 11/9/22.
//

/**
 * Create a data structure in either shared or regular memory.
 * Destroy the data structure when the unique pointer is deconstructed
 * */

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_

#include <labstor/memory/memory.h>
#include "data_structure.h"

namespace labstor::ipc {

template<typename T>
class unique_ptr {
 private:
  T *ptr_;

 public:
  unique_ptr(T *ptr) { ptr_ = ptr; }

  template<typename ...Args>
  unique_ptr(Args ...args) {
    ptr_ = new T(args...);
  }

  T* get() {
    return ptr_;
  }

  T& ref() {
    return *ptr_;
  }

  void operator=(unique_ptr<T> &&other) = delete;

  ~unique_ptr() {
    if (IS_SHM_SERIALIZEABLE(T)) {
      ptr_->Destroy();
    }
    delete ptr_;
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNIQUE_PTR_H_
