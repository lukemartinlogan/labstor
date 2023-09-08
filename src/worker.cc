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
      task->SetModuleComplete();
    }
    if (!task->IsMarked()) {
      HILOG(kDebug, "(node {}) Popped task: task_node={} task_state={} state_name={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_, exec->name_);
      task->SetMarked();
    }
    // Check if the task can execute
    if (!CheckTaskGroup(task, exec, task->task_node_)) {
      queue->Emplace(lane_id, p);
      continue;
    }
    // Check if this task is in a primary queue and needs to be moved
    if (queue->flags_.Any(QUEUE_PRIMARY) && !task->IsScheduled()) {
      task->SetScheduled();
      MultiQueue *real_queue = LABSTOR_CLIENT->GetQueue(QueueId(task->task_state_), false);
      real_queue->Emplace(task->lane_hash_, p);
      queue->Emplace(lane_id, p);
      continue;
    }
    // Disperse or execute task
    bool is_remote = task->domain_id_.IsRemote(LABSTOR_RPC->GetNumHosts(), LABSTOR_CLIENT->node_id_);
    if (!task->IsRunDisabled()) {
      if (is_remote) {
        auto ids = LABSTOR_RUNTIME->ResolveDomainId(task->domain_id_);
        LABSTOR_REMOTE_QUEUE->Disperse(task, exec, ids);
        task->DisableRun();
      } else if (!task->IsComplete()) {
        exec->Run(queue, task->method_, task);
      }
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      HILOG(kDebug, "(node {}) Ending task: task_node={} task_state={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_);
      RemoveTaskGroup();
      if (task->IsFireAndForget()) {
        if (!task->IsScheduled()) {
          LABSTOR_CLIENT->DelTask(task);
        } else if (queue->flags_.Any(QUEUE_PRIMARY)) {
          LABSTOR_CLIENT->DelTask(task);
        }
      }
      task->SetComplete();
    } else {
      queue->Emplace(lane_id, p);
    }
  }
}

void Worker::PollUnordered(u32 lane_id, MultiQueue *queue) {
  Task *task;
  hipc::Pointer p;
  // Unordered queue
  // TODO(llogan): make popping more fair
  for (int i = 0; i < 20; ++i) {
    if (!queue->Pop(lane_id, task, p)) {
      break;
    }
//    if (!task->task_flags_.Any(TASK_LONG_RUNNING)) {
//      HILOG(kInfo, "(node {}) Popping task: task_node={} task_state={}",
//            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_);
//    }
    TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
    if (!exec) {
      HELOG(kFatal, "(node {}) Could not find the task state: {}",
            LABSTOR_CLIENT->node_id_, task->task_state_);
      task->SetModuleComplete();
    }
    // Disperse or execute task
    bool is_remote = task->domain_id_.IsRemote(LABSTOR_RPC->GetNumHosts(), LABSTOR_CLIENT->node_id_);
    if (!task->IsRunDisabled()) {
      if (is_remote) {
        // flags_.Any(kGlobal | kSet) || (flags_.Any(kNode) && id_ != this_node)
        HILOG(kDebug, "Dispersing task task_state={} task_node={} is_global={} is_set={} is_node={} dom_id={} this_node={}",
              task->task_state_, task->task_node_,
              task->domain_id_.flags_.Any(DomainId::kGlobal),
              task->domain_id_.flags_.Any(DomainId::kSet),
              task->domain_id_.flags_.Any(DomainId::kNode),
              task->domain_id_.id_,
              LABSTOR_CLIENT->node_id_);
        auto ids = LABSTOR_RUNTIME->ResolveDomainId(task->domain_id_);
        LABSTOR_REMOTE_QUEUE->Disperse(task, exec, ids);
        task->DisableRun();
      } else if (!task->IsComplete()) {
        exec->Run(queue, task->method_, task);
      }
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      task->SetModuleComplete();
    }
    if (task->IsComplete()) {
      if (task->IsFireAndForget()) {
        LABSTOR_CLIENT->DelTask(task);
      }
    } else {
      queue->Emplace(lane_id, p);
    }
  }
}

void Worker::PollOrdered(u32 lane_id, MultiQueue *queue) {
  Task *task;
  hipc::Pointer p;
  // TODO(llogan): Consider popping the entire queue and storing a dependency graph
  // TODO(llogan): Consider implementing look-ahead
  for (int i = 0; i < 1; ++i) {
    if (!queue->Peek(lane_id, task, p, i)) {
      break;
    }
//    if (!task->task_flags_.Any(TASK_LONG_RUNNING)) {
//      HILOG(kInfo, "(node {}) Popping task: task_node={} task_state={}",
//            LABSTOR_CLIENT->node_id_, task->task_node_, task->task_state_);
//    }
    TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(task->task_state_);
    if (!exec) {
      HELOG(kFatal, "(node {}) Could not find the task state: {}",
            LABSTOR_CLIENT->node_id_, task->task_state_);
      task->SetModuleComplete();
    }
    // Disperse or execute task
    bool is_remote = task->domain_id_.IsRemote(LABSTOR_RPC->GetNumHosts(), LABSTOR_CLIENT->node_id_);
    if (!task->IsRunDisabled()) {
      if (is_remote) {
        auto ids = LABSTOR_RUNTIME->ResolveDomainId(task->domain_id_);
        LABSTOR_REMOTE_QUEUE->Disperse(task, exec, ids);
        task->DisableRun();
      } else if (!task->IsComplete()) {
        exec->Run(queue, task->method_, task);
      }
    }
    // Cleanup on task completion
    if (task->IsModuleComplete()) {
      task->SetModuleComplete();
    }
    if (task->IsComplete()) {
      queue->Pop(lane_id, task, p);
      if (task->IsFireAndForget()) {
        LABSTOR_CLIENT->DelTask(task);
      }
    }
  }
}

}  // namespace labstor