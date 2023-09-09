//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "proc_queue/proc_queue.h"

namespace labstor::proc_queue {

class Server : public TaskLib {
 public:
  Server() = default;

  void Construct(MultiQueue *queue, ConstructTask *task) {
    task->SetModuleComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetModuleComplete();
  }

  void Custom(MultiQueue *queue, CustomTask *task) {
    task->SetModuleComplete();
  }

 public:
#include "proc_queue/proc_queue_lib_exec.h"
};

}  // namespace labstor::proc_queue

LABSTOR_TASK_CC(labstor::proc_queue::Server, "proc_queue");
