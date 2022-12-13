//
// Created by lukemartinlogan on 11/3/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_MANAGER_H_

#include "thread.h"
#include "thread_factory.h"
#include <labstor/constants/data_structure_singleton_macros.h>

namespace labstor {

class ThreadManager {
 public:
  ThreadType type_;
  std::unique_ptr<ThreadStatic> thread_static_;

  ThreadManager() : type_(ThreadType::kPthread) {}

  void SetThreadType(ThreadType type) {
    type_ = type;
  }

  ThreadStatic* GetThreadStatic() {
    if (!thread_static_) {
      thread_static_ = ThreadStaticFactory::Get(type_);
    }
    return thread_static_.get();
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_MANAGER_H_
