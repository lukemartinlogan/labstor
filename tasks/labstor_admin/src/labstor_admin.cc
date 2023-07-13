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

  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        break;
      }
      case Method::kDestruct: {
        break;
      }
      case Method::kCreateQueue: {
        CreateQueue(queue, reinterpret_cast<CreateQueueTask *>(task));
        break;
      }
      case Method::kDestroyQueue: {
        DestroyQueue(queue, reinterpret_cast<DestroyQueueTask *>(task));
        break;
      }
      case Method::kRegisterTaskLib: {
        RegisterTaskLib(queue, reinterpret_cast<RegisterTaskLibTask *>(task));
        break;
      }
      case Method::kDestroyTaskLib: {
        DestroyTaskLib(queue, reinterpret_cast<DestroyTaskLibTask *>(task));
        break;
      }
      case Method::kCreateTaskState: {
        CreateTaskState(queue, reinterpret_cast<CreateTaskStateTask *>(task));
        break;
      }
      case Method::kGetTaskStateId: {
        GetTaskStateId(queue, reinterpret_cast<GetTaskStateIdTask *>(task));
        break;
      }
      case Method::kDestroyTaskState: {
        DestroyTaskState(queue, reinterpret_cast<DestroyTaskStateTask *>(task));
        break;
      }
      case Method::kStopRuntime: {
        StopRuntime(queue, reinterpret_cast<StopRuntimeTask *>(task));
        break;
      }
      case Method::kSetWorkOrchestratorQueuePolicy: {
        SetWorkOrchestratorQueuePolicy(queue,
                                       reinterpret_cast<SetWorkOrchestratorQueuePolicyTask *>(task));
        break;
      }
      case Method::kSetWorkOrchestratorProcessPolicy: {
        SetWorkOrchestratorProcessPolicy(
            queue, reinterpret_cast<SetWorkOrchestratorProcessPolicyTask *>(task));
        break;
      }
    }
  }

  void CreateQueue(MultiQueue *queue, CreateQueueTask *task) {
    QueueId id = task->id_;
    u32 max_lanes = task->max_lanes_;
    u32 num_lanes = task->num_lanes_;
    u32 depth = task->depth_;
    bitfield32_t flags = task->flags_;
    LABSTOR_QM_RUNTIME->CreateQueue(id, max_lanes, num_lanes, depth, flags);
    task->SetComplete();
  }

  void DestroyQueue(MultiQueue *queue, DestroyQueueTask *task) {
    QueueId id = task->id_;
    LABSTOR_QM_RUNTIME->DestroyQueue(id);
    task->SetComplete();
  }

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
    if (!LABSTOR_TASK_REGISTRY->GetTaskStateId(state_name).IsNull()) {
      return;
    }
    if (task->id_.IsNull()) {
      task->id_ = LABSTOR_TASK_REGISTRY->CreateTaskStateId(task->domain_id_.id_);
    }
    LABSTOR_TASK_REGISTRY->CreateTaskState(lib_name.c_str(),
                                           state_name.c_str(),
                                           task->domain_id_,
                                           task->id_,
                                           task);
    task->SetComplete();
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
    LABSTOR_WORK_ORCHESTRATOR->FinalizeRuntime();
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
    queue_sched_ = queue->Allocate<Task>(
        LABSTOR_CLIENT->main_alloc_, p,
        0, task->policy_id_,
        SchedulerMethod::kSchedule, DomainId::GetLocal(),
        bitfield32_t(TASK_LONG_RUNNING));
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
    proc_sched_ = queue->Allocate<Task>(
        LABSTOR_CLIENT->main_alloc_, p,
        0, task->policy_id_,
        SchedulerMethod::kSchedule, DomainId::GetLocal(),
        bitfield32_t(TASK_LONG_RUNNING));
    queue->Emplace(0, p);
    task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::Admin::Server, "labstor_admin");
