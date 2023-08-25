//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "worch_proc_round_robin/worch_proc_round_robin.h"

namespace labstor::worch_proc_round_robin {

class Server : public TaskLib {
 public:
  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kSchedule: {
        Schedule(queue, task);
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    if (task) {
      task->SetComplete();
    }
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    if (task) {
      task->SetComplete();
    }
  }

  void Schedule(MultiQueue *queue, Task *task) {
    int rr = 0;
    for (Worker &worker : LABSTOR_WORK_ORCHESTRATOR->workers_) {
      worker.SetCpuAffinity(rr % HERMES_SYSTEM_INFO->ncpu_);
      ++rr;
    }
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::worch_proc_round_robin::Server, "worch_proc_round_robin");
