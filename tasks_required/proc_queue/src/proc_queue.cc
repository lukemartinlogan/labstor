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

  void Push(MultiQueue *queue, PushTask *task) {
    switch (task->phase_) {
      case PushTaskPhase::kSchedule: {
        MultiQueue *real_queue = LABSTOR_CLIENT->GetQueue(QueueId(task->task_state_));
        real_queue->Emplace(task->subtask_ptr_->lane_hash_, task->subtask_);
        task->phase_ = PushTaskPhase::kWaitSchedule;
      }
      case PushTaskPhase::kWaitSchedule: {
        if (!task->subtask_ptr_->IsComplete()) {
          return;
        }
        task->SetModuleComplete();
      }
    }
  }

 public:
#include "proc_queue/proc_queue_lib_exec.h"
};

}  // namespace labstor::proc_queue

LABSTOR_TASK_CC(labstor::proc_queue::Server, "proc_queue");
