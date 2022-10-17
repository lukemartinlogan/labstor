//
// Created by lukemartinlogan on 10/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_

#include "thread.h"
#include "pthread.h"

namespace labstor {

template<typename BIND>
class ThreadFactory {
 private:
  ThreadType type_;
  BIND bind_;

 public:
  ThreadFactory(ThreadType type, BIND bind) : type_(type), bind_(bind) {}
  std::unique_ptr<Thread> Get() {
    switch(type_) {
      case ThreadType::kPthread: return std::make_unique<Pthread<BIND>>(bind_);
      default: return nullptr;
    }
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_
