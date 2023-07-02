//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"

namespace labstor::Admin {

class Server : public TaskLib {
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
      case Method::kCreateTaskExecutor: {
        CreateTaskExecutor(queue, reinterpret_cast<CreateTaskExecutorTask *>(task));
        break;
      }
      case Method::kGetTaskExecutorId: {
        GetTaskExecutorId(queue, reinterpret_cast<GetTaskExecutorIdTask *>(task));
        break;
      }
      case Method::kDestroyTaskExecutor: {
        DestroyTaskExecutor(queue, reinterpret_cast<DestroyTaskExecutorTask *>(task));
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

  void CreateTaskExecutor(MultiQueue *queue, CreateTaskExecutorTask *task) {
    std::string lib_name = task->lib_name_->str();
    std::string exec_name = task->exec_name_->str();
    if (!LABSTOR_TASK_REGISTRY->GetTaskExecutorId(exec_name).IsNull()) {
      return;
    }
    if (task->id_.IsNull()) {
      task->id_ = LABSTOR_TASK_REGISTRY->CreateTaskExecutorId(task->node_id_);
    }
    LABSTOR_TASK_REGISTRY->CreateTaskExecutor(lib_name.c_str(),
                                              exec_name.c_str(),
                                              task->node_id_,
                                              task->id_,
                                              task);
    task->SetComplete();
  }

  void GetTaskExecutorId(MultiQueue *queue, GetTaskExecutorIdTask *task) {
    std::string exec_name = task->exec_name_->str();
    task->id_ = LABSTOR_TASK_REGISTRY->GetTaskExecutorId(exec_name);
    task->SetComplete();
  }

  void DestroyTaskExecutor(MultiQueue *queue, DestroyTaskExecutorTask *task) {
    LABSTOR_TASK_REGISTRY->DestroyTaskExecutor(task->id_);
    task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::Admin::Server, "labstor_admin");
