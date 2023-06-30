//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"

namespace labstor {

class Admin : public TaskLib {
  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case AdminMethod::kConstruct: {
        break;
      }
      case AdminMethod::kDestruct: {
        break;
      }
      case AdminMethod::kCreateQueue: {
        break;
      }
      case AdminMethod::kCreateQueueAndId: {
        break;
      }
      case AdminMethod::kDestroyQueue: {
        break;
      }
      case AdminMethod::kRegisterTaskLib: {
        break;
      }
      case AdminMethod::kUnregisterTaskLib: {
        break;
      }
      case AdminMethod::kSpawnTaskExecutor: {
        break;
      }
      case AdminMethod::kDestroyTaskExecutor: {
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

  void CreateQueueAndId(MultiQueue *queue, CreateQueueTask *task) {
    u32 max_lanes = task->max_lanes_;
    u32 num_lanes = task->num_lanes_;
    u32 depth = task->depth_;
    bitfield32_t flags = task->flags_;
    QueueId id = LABSTOR_QM_RUNTIME->CreateQueue(max_lanes, num_lanes, depth, flags);
    task->id_ = id;
    task->SetComplete();
  }

  void DestroyQueue(MultiQueue *queue, DestroyQueueTask *task) {
    QueueId id = task->id_;
    LABSTOR_QM_RUNTIME->DestroyQueue(id);
    task->SetComplete();
  }

  void RegisterTaskLib(MultiQueue *queue, RegisterTaskLibTask *task) {
    TaskLib *task_lib = task->task_lib_;
    LABSTOR_TASK_REGISTRY->RegisterTaskLib(task_lib);
    task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::Admin, "admin");
