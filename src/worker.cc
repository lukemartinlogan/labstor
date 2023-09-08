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
    if (!queue->IsPrimary()) {
      PollGrouped(lane_id, queue);
    } else {
      PollPrimary(lane_id, queue);
    }
  }
}

void Worker::PollPrimary(u32 lane_id, MultiQueue *queue) {
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
    // Schedule the primary task on a new queue if it's ready
    // Ensure the task group is acquired
    if (BeginPrimaryTask(task, exec, task->task_node_)) {
      HILOG(kDebug,
            "(node {}) Popped task: task_node={} task_state={} state_name={} lane={} queue={} worker={} primary=true",
            LABSTOR_CLIENT->node_id_,
            task->task_node_,
            task->task_state_,
            exec->name_,
            lane_id,
            queue->id_,
            id_);
      task->UnsetMarked();
      task->SetPrimary();
      task->task_node_.node_depth_ += 1;
      MultiQueue *real_queue = LABSTOR_CLIENT->GetQueue(QueueId(task->task_state_), false);
      real_queue->Emplace(task->lane_hash_, p, true);
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      HILOG(kDebug, "(node {}) Ending task: task_node={} task_state={} lane={} queue={} worker={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_, lane_id, queue->id_, id_);
      RemoveTaskGroup(task, exec);
      if (task->IsFireAndForget()) {
        LABSTOR_CLIENT->DelTask(task);
      } else {
        task->SetComplete();
      }
    } else {
      queue->Emplace(lane_id, p, true);
    }
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
      if (!task->IsPrimary()) {
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