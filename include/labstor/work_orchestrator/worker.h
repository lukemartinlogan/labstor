//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_

#include "labstor/labstor_types.h"

namespace labstor {

class Worker {
 public:
  u32 id_;
  u32 numa_node_;

 public:
  Worker(u32 id) {
    id_ = id;
  }

  void Loop();

 private:
  void Run();
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
