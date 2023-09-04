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

  void GetOrCreateTaskStateId(MultiQueue *queue, GetOrCreateTaskStateIdTask *task) {
    std::string state_name = task->state_name_->str();
    task->id_ = LABSTOR_TASK_REGISTRY->GetOrCreateTaskStateId(state_name);
    task->SetComplete();
  }

  void CreateTaskState(MultiQueue *queue, CreateTaskStateTask *task) {
    switch (task->phase_) {
      case CreateTaskStatePhase::kIdAllocStart: {
        std::string lib_name = task->lib_name_->str();
        std::string state_name = task->state_name_->str();
        // Check local registry for task state
        TaskState *task_state = LABSTOR_TASK_REGISTRY->GetTaskState(state_name, task->id_);
        if (task_state) {
          task->id_ = task_state->id_;
          task->SetComplete();
          return;
        }
        // Check global registry for task state
        if (task->id_.IsNull()) {
          if (task->domain_id_ == DomainId::GetLocal()) {
            task->id_ = LABSTOR_TASK_REGISTRY->GetOrCreateTaskStateId(state_name);
            task->phase_ = CreateTaskStatePhase::kStateCreate;
          } else {
            // u64 hash = std::hash<std::string>{}(state_name);
            // DomainId domain = DomainId::GetNode(HASH_TO_NODE_ID(hash));
            DomainId domain = DomainId::GetNode(1);
            task->get_id_task_ = LABSTOR_ADMIN->AsyncGetOrCreateTaskStateId(
                task->task_node_, domain, state_name);
            task->phase_ = CreateTaskStatePhase::kIdAllocWait;
          }
        } else {
          task->phase_ = CreateTaskStatePhase::kStateCreate;
        }
        return;
      }
      case CreateTaskStatePhase::kIdAllocWait: {
        if (!task->get_id_task_->IsComplete()) {
          return;
        }
        task->id_ = task->get_id_task_->id_;
        task->phase_ = CreateTaskStatePhase::kStateCreate;
        LABSTOR_CLIENT->DelTask(task->get_id_task_);
      }
      case CreateTaskStatePhase::kStateCreate: {
        std::string lib_name = task->lib_name_->str();
        std::string state_name = task->state_name_->str();
        HILOG(kInfo, "(node {}) Creating task state {} with id {}",
              LABSTOR_QM_CLIENT->node_id_, state_name, task->id_);

        // Verify the state doesn't exist
        if (LABSTOR_TASK_REGISTRY->TaskStateExists(task->id_)) {
          HILOG(kInfo, "(node {}) The task state {} with id {} exists",
                LABSTOR_QM_CLIENT->node_id_, state_name, task->id_);
          task->SetComplete();
          return;
        }

        // The state is being created
        // NOTE(llogan): this does NOT return since task creations can have phases
        task->method_ = Method::kConstruct;

        // Create the task queue for the state
        QueueId qid(task->id_);
        if (task->queue_max_lanes_ > 0) {
          LABSTOR_QM_RUNTIME->CreateQueue(
              qid, task->queue_max_lanes_, task->queue_num_lanes_,
              task->queue_depth_, task->queue_flags_);
          HILOG(kInfo, "(node {}) Allocated task state {} with id {}",
                LABSTOR_QM_CLIENT->node_id_, state_name, task->task_state_);
        }

        // Begin creating the task state
        task->phase_ = 0;
        task->task_state_ = task->id_;
        bool ret = LABSTOR_TASK_REGISTRY->CreateTaskState(
            lib_name.c_str(),
            state_name.c_str(),
            task->id_,
            task);
        if (!ret) {
          task->SetComplete();
          return;
        }
        HILOG(kInfo, "(node {}) Allocated task state {} with id {}",
              LABSTOR_QM_CLIENT->node_id_, state_name, task->task_state_);
      }
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
    HILOG(kInfo, "Stopping (server mode)");
    LABSTOR_WORK_ORCHESTRATOR->FinalizeRuntime();
    LABSTOR_THALLIUM->StopThisDaemon();
    task->SetComplete();
  }

  void SetWorkOrchestratorQueuePolicy(MultiQueue *queue, SetWorkOrchestratorQueuePolicyTask *task) {
    if (queue_sched_) {
      queue_sched_->SetExternalComplete();
    }
    if (queue_sched_ && !queue_sched_->IsComplete()) {
      return;
    }
    hipc::Pointer p;
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
