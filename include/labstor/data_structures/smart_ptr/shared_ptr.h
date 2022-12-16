//
// Created by lukemartinlogan on 11/9/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHARED_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHARED_PTR_H_

#include "labstor/memory/memory.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

template<typename T>
class shared_ptr : public ShmDataStructure<shared_ptr<T>> {
 private:
  T *ptr_;

 public:
  shared_ptr(T *ptr) { ptr_ = ptr; }

  template<typename ...Args>
  shared_ptr(Args&& ...args) {
    ptr_ = new T(std::forward<Args>(args)...);
  }

  T* get() {
    return ptr_;
  }

  T& ref() {
    return *ptr_;
  }

  void operator=(shared_ptr<T> &&other) {

  }

  ~shared_ptr() {
    if (IS_SHM_SERIALIZEABLE(T)) {
      ptr_->Destroy();
    }
    delete ptr_;
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHARED_PTR_H_
