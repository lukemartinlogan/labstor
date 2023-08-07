//
// Created by lukemartinlogan on 6/27/23.
//

#include "labstor/api/labstor_runtime.h"
#include "labstor/work_orchestrator/worker.h"
#include "labstor/work_orchestrator/work_orchestrator.h"

namespace labstor {

// Should queues work across task states?

void Worker::Loop() {
  pid_ = gettid();
  while (LABSTOR_WORK_ORCHESTRATOR->IsAlive()) {
    Run();
//    try {
//      Run();
//    } catch (hshm::Error &e) {
//      e.print();
//      exit(1);
//    }
    // Yield();
  }
  Run();
}

void Worker::Run() {
  if (poll_queues_.size() > 0) {
    _PollQueues();
  }
  if (relinquish_queues_.size() > 0) {
    _RelinquishQueues();
  }

  /** Peek a set of tasks to activate */
  for (WorkEntry &entry : work_queue_) {
    Task *task;
    hipc::Pointer p;

    // Get the queue
    auto &lane_id = entry.lane_;
    auto &queue = entry.queue_;
    constexpr int iter = 20;

    // Create the active task queue
    if (active_tasks_.find(entry) == active_tasks_.end()) {
      active_tasks_[entry] = std::queue<Task*>();
    }

    // Verify there is space in the active queue
    auto &active_queue = active_tasks_[entry];
    for (int i = active_queue.size(); i < iter; ++i) {
      if (!queue->Pop(lane_id, task, p)) {
        break;
      }
      if (queue->flags_.Any(QUEUE_UNORDERED) ||
          task->task_flags_.Any(TASK_UNORDERED) ||
          (i == 0 && active_tasks_[entry].empty())) {
        active_tasks_[entry].push(task);
      }
    }
  }

  /** Run active tasks */
  Task *task;
  hipc::Pointer p;
  for (WorkEntry &entry : work_queue_) {
    auto &lane_id = entry.lane_;
    auto &queue = entry.queue_;
    auto &active_queue = active_tasks_[entry];
    size_t queue_size = active_queue.size();
    for (size_t i = 0; i < queue_size; ++i) {
      task = active_queue.front();
      active_queue.pop();

      TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
      if (!exec) {
        HELOG(kError, "Could not find the task state: {}", task->task_state_);
        task->SetComplete();
      }
      if (!task->IsComplete()) {
        exec->Run(queue, task->method_, task);
      }
      if (task->IsExternalComplete()) {
        task->SetComplete();
      }

      // Either release task or re-activate
      if (task->IsComplete()) {
        if (task->IsFireAndForget()) {
          LABSTOR_CLIENT->DelTask(task);
        }
      } else {
        active_queue.push(task);
      }
    }
  }
}

}  // namespace labstor