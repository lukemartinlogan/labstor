//
// Created by lukemartinlogan on 6/27/23.
//

#include "labstor/api/labstor_runtime.h"
#include "labstor/work_orchestrator/worker.h"
#include "labstor/work_orchestrator/work_orchestrator.h"

namespace labstor {

void Worker::Loop() {
  pid_ = gettid();
  while (LABSTOR_WORK_ORCHESTRATOR->IsAlive()) {
    try {
      Run();
    } catch (hshm::Error &e) {
      e.print();
      exit(1);
    }
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
  for (auto &[lane_id, queue] : work_queue_) {
    PollGrouped(lane_id, queue);
  }
}

void Worker::PollGrouped(u32 lane_id, MultiQueue *queue) {
  Task *task;
  hipc::Pointer p;
  int head = 0;
  for (int i = 0; i < 1024; ++i) {
    // Get the task message
    if (!queue->Peek(lane_id, task, p, i)) {
      break;
    }
    // Get the task state
    TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
    if (!exec) {
      HELOG(kFatal, "(node {}) Could not find the task state: {}",
            LABSTOR_CLIENT->node_id_, task->task_state_);
      EndTask(lane_id, queue, task, exec, i, head);
      continue;
    }
    // Attempt to run the task if it's ready and runnable
    if (!task->IsModuleComplete() && !task->IsRunDisabled() && CheckTaskGroup(task, exec, task->task_node_)) {
      if (!task->IsMarked()) {
        HILOG(kDebug, "(node {}) Popped task: task_node={} task_state={} state_name={} lane={} queue={} worker={} primary=false",
              LABSTOR_CLIENT->node_id_, task->task_node_,
              task->task_state_, exec->name_, lane_id, queue->id_, id_);
        task->SetMarked();
      }
      bool is_remote = task->domain_id_.IsRemote(LABSTOR_RPC->GetNumHosts(), LABSTOR_CLIENT->node_id_);
      // Execute or schedule task
      if (is_remote) {
        auto ids = LABSTOR_RUNTIME->ResolveDomainId(task->domain_id_);
        LABSTOR_REMOTE_QUEUE->Disperse(task, exec, ids);
        task->DisableRun();
      } else {
        task->SetStarted();
        exec->Run(queue, task->method_, task);
      }
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      HILOG(kDebug, "(node {}) Ending task: task_node={} task_state={} lane={} queue={} worker={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_, lane_id, queue->id_, id_);
      RemoveTaskGroup(task, exec);
      EndTask(lane_id, queue, task, exec, i, head);
    } else if (task->IsUnordered()) {
      queue->Pop(lane_id, task, p);
      queue->Emplace(lane_id, p);
    }
  }
}

}  // namespace labstor