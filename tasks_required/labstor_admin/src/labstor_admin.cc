//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "labstor/work_orchestrator/scheduler.h"

namespace labstor::Admin {

class Server : public TaskLib {
 public:
  Task *queue_sched_;
  Task *proc_sched_;

 public:
  Server() : queue_sched_(nullptr), proc_sched_(nullptr) {}

  void RegisterTaskLib(MultiQueue *queue, RegisterTaskLibTask *task) {
    std::string lib_name = task->lib_name_->str();
    LABSTOR_TASK_REGISTRY->RegisterTaskLib(lib_name);
    task->SetComplete();
  }

  void DestroyTaskLib(MultiQueue *queue, DestroyTaskLibTask *task) {
    std::string lib_name = task->lib_name_->str();
    LABSTOR_TASK_REGISTRY->DestroyTaskLib(lib_name);
    task->SetComplete();
  }

  void CreateTaskState(MultiQueue *queue, CreateTaskStateTask *task) {
    std::string lib_name = task->lib_name_->str();
    std::string state_name = task->state_name_->str();
    TaskState *task_state = LABSTOR_TASK_REGISTRY->GetTaskState(state_name, task->id_);
    if (task_state) {
      // The tasks exists and is initialized
      task->id_ = task_state->id_;
      task->SetComplete();
    } else {
      // The state is being created
      // NOTE(llogan): this does NOT return since task creations can have phases
      task->method_ = Method::kConstruct;

      // Create the task queue for the state
      TaskStateId new_id = task->id_;
      if (new_id.IsNull()) {
        new_id = LABSTOR_TASK_REGISTRY->CreateTaskStateId();
      }
      QueueId qid(new_id);
      if (task->queue_max_lanes_ > 0) {
        LABSTOR_QM_RUNTIME->CreateQueue(
            qid, task->queue_max_lanes_, task->queue_num_lanes_,
            task->queue_depth_, task->queue_flags_);
      }

      // Begin creating the task state
      LABSTOR_TASK_REGISTRY->CreateTaskState(
          lib_name.c_str(),
          state_name.c_str(),
          new_id,
          task);
      task->task_state_ = task->id_;
    }
  }

  void GetTaskStateId(MultiQueue *queue, GetTaskStateIdTask *task) {
    std::string state_name = task->state_name_->str();
    task->id_ = LABSTOR_TASK_REGISTRY->GetTaskStateId(state_name);
    task->SetComplete();
  }

  void DestroyTaskState(MultiQueue *queue, DestroyTaskStateTask *task) {
    LABSTOR_TASK_REGISTRY->DestroyTaskState(task->id_);
    task->SetComplete();
  }

  void StopRuntime(MultiQueue *queue, StopRuntimeTask *task) {
    task->SetComplete();
    LABSTOR_WORK_ORCHESTRATOR->FinalizeRuntime();
  }

  void SetWorkOrchestratorQueuePolicy(MultiQueue *queue, SetWorkOrchestratorQueuePolicyTask *task) {
    if (queue_sched_) {
      queue_sched_->SetExternalComplete();
    }
    if (queue_sched_ && !queue_sched_->IsComplete()) {
      return;
    }
    hipc::Pointer p;
    /*
       u32 lane_hash,
       u32 method,
       bitfield32_t task_flags
     * */

    queue_sched_ = LABSTOR_CLIENT->NewTask<Task>(
        p, task->task_node_, DomainId::GetLocal(), task->policy_id_,
        0, SchedulerMethod::kSchedule, bitfield32_t(TASK_LONG_RUNNING));
    queue->Emplace(0, p);
    task->SetComplete();
  }

  void SetWorkOrchestratorProcessPolicy(MultiQueue *queue, SetWorkOrchestratorProcessPolicyTask *task) {
    if (proc_sched_) {
      proc_sched_->SetExternalComplete();
    }
    if (proc_sched_ && !proc_sched_->IsComplete()) {
      return;
    }
    hipc::Pointer p;
    proc_sched_ = LABSTOR_CLIENT->NewTask<Task>(
        p, task->task_node_, DomainId::GetLocal(), task->policy_id_,
        0, SchedulerMethod::kSchedule,
        bitfield32_t(TASK_LONG_RUNNING));
    queue->Emplace(0, p);
    task->SetComplete();
  }

 public:
#include "labstor_admin/labstor_admin_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::Admin::Server, "labstor_admin");
