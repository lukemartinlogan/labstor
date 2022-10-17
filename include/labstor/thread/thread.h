//
// Created by lukemartinlogan on 10/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_H_

#include <vector>
#include <cstdint>
#include <memory>

namespace labstor {

enum class ThreadType {
  kPthread
};

class Thread {
 public:
  virtual void Pause() = 0;
  virtual void Resume() = 0;
  virtual void Join() = 0;
  void SetAffinity(int cpu_id) { SetAffinity(cpu_id, 1); }
  virtual void SetAffinity(int cpu_start, int count) = 0;
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_H_
