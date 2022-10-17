//
// Created by lukemartinlogan on 10/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_PTHREAD_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_PTHREAD_H_

#include "thread.h"
#include <errno.h>
#include <labstor/util/errors.h>

namespace labstor {

template<typename BIND> class Pthread;

template<typename BIND>
struct PthreadParams {
  BIND bind_;
  Pthread<BIND> *pthread_;

  PthreadParams(Pthread<BIND> *pthread, BIND bind) :
    bind_(bind),
    pthread_(pthread) {}
};

template<typename BIND>
class Pthread : public Thread {
 private:
  pthread_t pthread_;

 public:
  bool started_;

 public:
  Pthread(BIND bind) : started_(false), pthread_(-1) {
    PthreadParams<BIND> params(this, bind);
    int ret = pthread_create(&pthread_, nullptr,
                             DoWork,
                             &params);
    if (ret != 0) {
      throw PTHREAD_CREATE_FAILED.format();
    }
    while(!started_);
  }

  void Pause() override {}

  void Resume() override {}

  void Join() override {
    void *ret;
    pthread_join(pthread_, &ret);
  }

  void SetAffinity(int cpu_start, int count) override {
    /*cpu_set_t cpus[n_cpu_];
    CPU_ZERO(cpus);
    CPU_SET(cpu_id, cpus);
    pthread_setaffinity_np_safe(n_cpu_, cpus);*/
  }

 private:
  static void* DoWork(void *void_params) {
    auto params = (*reinterpret_cast<PthreadParams<BIND>*>(void_params));
    params.pthread_->started_ = true;
    params.bind_();
    return nullptr;
  }

  inline void pthread_setaffinity_np_safe(int n_cpu, cpu_set_t *cpus) {
    int ret = pthread_setaffinity_np(pthread_, n_cpu, cpus);
    if(ret != 0) {
      throw INVALID_AFFINITY.format(strerror(ret));
    }
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_PTHREAD_H_
