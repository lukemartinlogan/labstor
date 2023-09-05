//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "worch_proc_round_robin/worch_proc_round_robin.h"

namespace labstor::worch_proc_round_robin {

class Server : public TaskLib {
 public:
  void Construct(MultiQueue *queue, ConstructTask *task) {
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  void Schedule(MultiQueue *queue, ScheduleTask *task) {
    int rr = 0;
    for (Worker &worker : LABSTOR_WORK_ORCHESTRATOR->workers_) {
      worker.SetCpuAffinity(rr % HERMES_SYSTEM_INFO->ncpu_);
      ++rr;
    }
  }

#include "worch_proc_round_robin/worch_proc_round_robin_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::worch_proc_round_robin::Server, "worch_proc_round_robin");
