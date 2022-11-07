//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_RWQUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_RWQUEUE_H_

#include "labstor/data_structures/lockless/array_queue.h"
#include "labstor/data_structures/lockless/array.h"
#include <labstor/introspect/system_info.h>
#include <labstor/thread/thread_manager.h>

namespace labstor {

struct RwInfo {
  pid_t pid_;
  tid_t tid_;
  time_t timestamp_;
};

typedef labstor::array_queue<RwInfo> RwLane;
typedef labstor::array<RwLane> RwQueue;

class RwQueue : ShmSerializeable<> {
 private:
  RwQueue read_;
  RwQueue write_;
  time_t timeout_;

 public:
  void ReadLock();
  void ReadUnlock();

  void WriteLock();
  void WriteUnlock();
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_RWQUEUE_H_
