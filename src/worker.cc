//
// Created by lukemartinlogan on 6/27/23.
//

#include "labstor/api/labstor_runtime.h"
#include "labstor/work_orchestrator/worker.h"
#include "labstor/work_orchestrator/work_orchestrator.h"

namespace labstor {

// Should queues work across task states?

void Worker::Loop() {
  while (LABSTOR_WORK_ORCHESTRATOR->IsAlive()) {
    Run();
  }
  Run();
}

void Worker::Run() {
  for (auto &[lane_id, queue] : work_queue_) {
    Task *task;
    hipc::Pointer p;
    if (poll_queues_.size() > 0) {
      _PollQueues();
    }
    if (relinquish_queues_.size() > 0) {
      _RelinquishQueues();
    }

    // TODO(llogan): make polling more fair
    while (queue->Pop(lane_id, task, p)) {
      TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
      exec->Run(queue, task->method_, task);
      if (task->IsExternalComplete()) {
        task->SetComplete();
        continue;
      }
      if (!task->IsComplete()) {
        queue->Emplace(lane_id, p);
      }
    }
  }
}

}  // namespace labstor