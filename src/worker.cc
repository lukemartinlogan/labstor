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
//    if (queue->flags_.Any(QUEUE_UNORDERED)) {
//      PollUnordered(lane_id, queue);
//    } else {
//      PollOrdered(lane_id, queue);
//    }
  }
}

void Worker::PollGrouped(u32 lane_id, MultiQueue *queue) {
  Task *task;
  hipc::Pointer p;
  for (int i = 0; i < 1024; ++i) {
    // Get the task message
    if (!queue->Pop(lane_id, task, p)) {
      break;
    }
    // Get the task state
    TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
    if (!exec) {
      HELOG(kFatal, "(node {}) Could not find the task state: {}",
            LABSTOR_CLIENT->node_id_, task->task_state_);
      task->SetComplete();
      if (task->IsFireAndForget()) {
        LABSTOR_CLIENT->DelTask(task);
      }
      continue;
    }
    if (!task->IsMarked() || (!task->IsPrimary() && queue->IsPrimary())) {
      HILOG(kDebug, "(node {}) Popped task: task_node={} task_state={} state_name={} lane={} queue={} worker={}",
            LABSTOR_CLIENT->node_id_, task->task_node_,
            task->task_state_, exec->name_, lane_id, queue->id_, id_);
      task->SetMarked();
    }
    // Attempt to run the task if it's ready and runnable
    if (!task->IsModuleComplete() && !task->IsRunDisabled()) {
      if (queue->IsPrimary()) {
        // Check if intermediate task is ready to run
        if (!BeginPrimaryTask(task, exec, task->task_node_)) {
          queue->Emplace(lane_id, p, true);
          continue;
        }
        // Schedule the primary task on a new local queue
        task->UnsetMarked();
        task->SetPrimary();
        task->task_node_.node_depth_ += 1;
        MultiQueue *real_queue = LABSTOR_CLIENT->GetQueue(QueueId(task->task_state_), false);
        real_queue->Emplace(task->lane_hash_, p, false);
        queue->Emplace(lane_id, p, true);
        continue;
      } else {
        // Check if intermediate task is ready to run
        if (!CheckTaskGroup(task, exec, task->task_node_)) {
          queue->Emplace(lane_id, p, false);
          continue;
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
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      HILOG(kDebug, "(node {}) Ending task: task_node={} task_state={} lane={} queue={} worker={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_, lane_id, queue->id_, id_);
      RemoveTaskGroup(task, exec);
      if (!task->IsPrimary() || queue->IsPrimary()) {
        if (task->IsFireAndForget()) {
          LABSTOR_CLIENT->DelTask(task);
        } else {
          task->SetComplete();
        }
      }
    } else {
      queue->Emplace(lane_id, p);
    }
  }
}

}  // namespace labstor