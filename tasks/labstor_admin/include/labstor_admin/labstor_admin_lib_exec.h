#ifndef LABSTOR_LABSTOR_ADMIN_LIB_EXEC_H_
#define LABSTOR_LABSTOR_ADMIN_LIB_EXEC_H_

/** Execute a task */
void Run(MultiQueue *queue, u32 method, Task *task) override {
  switch (method) {
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
      SetWorkOrchestratorQueuePolicy(queue, reinterpret_cast<SetWorkOrchestratorQueuePolicyTask *>(task));
      break;
    }
    case Method::kSetWorkOrchestratorProcessPolicy: {
      SetWorkOrchestratorProcessPolicy(queue, reinterpret_cast<SetWorkOrchestratorProcessPolicyTask *>(task));
      break;
    }
  }
}
/** Serialize a task when initially pushing into remote */
std::vector<DataTransfer> SaveStart(u32 method, BinaryOutputArchive<true> &ar, Task *task) override {
  switch (method) {
    case Method::kRegisterTaskLib: {
      ar << *reinterpret_cast<RegisterTaskLibTask*>(task);
      break;
    }
    case Method::kDestroyTaskLib: {
      ar << *reinterpret_cast<DestroyTaskLibTask*>(task);
      break;
    }
    case Method::kCreateTaskState: {
      ar << *reinterpret_cast<CreateTaskStateTask*>(task);
      break;
    }
    case Method::kGetTaskStateId: {
      ar << *reinterpret_cast<GetTaskStateIdTask*>(task);
      break;
    }
    case Method::kDestroyTaskState: {
      ar << *reinterpret_cast<DestroyTaskStateTask*>(task);
      break;
    }
    case Method::kStopRuntime: {
      ar << *reinterpret_cast<StopRuntimeTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorQueuePolicy: {
      ar << *reinterpret_cast<SetWorkOrchestratorQueuePolicyTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorProcessPolicy: {
      ar << *reinterpret_cast<SetWorkOrchestratorProcessPolicyTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when popping from remote queue */
TaskPointer LoadStart(u32 method, BinaryInputArchive<true> &ar) override {
  TaskPointer task_ptr;
  switch (method) {
    case Method::kRegisterTaskLib: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<RegisterTaskLibTask>(task_ptr.p_);
      ar >> *reinterpret_cast<RegisterTaskLibTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestroyTaskLib: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestroyTaskLibTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestroyTaskLibTask*>(task_ptr.task_);
      break;
    }
    case Method::kCreateTaskState: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<CreateTaskStateTask>(task_ptr.p_);
      ar >> *reinterpret_cast<CreateTaskStateTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetTaskStateId: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetTaskStateIdTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetTaskStateIdTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestroyTaskState: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestroyTaskStateTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestroyTaskStateTask*>(task_ptr.task_);
      break;
    }
    case Method::kStopRuntime: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<StopRuntimeTask>(task_ptr.p_);
      ar >> *reinterpret_cast<StopRuntimeTask*>(task_ptr.task_);
      break;
    }
    case Method::kSetWorkOrchestratorQueuePolicy: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<SetWorkOrchestratorQueuePolicyTask>(task_ptr.p_);
      ar >> *reinterpret_cast<SetWorkOrchestratorQueuePolicyTask*>(task_ptr.task_);
      break;
    }
    case Method::kSetWorkOrchestratorProcessPolicy: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<SetWorkOrchestratorProcessPolicyTask>(task_ptr.p_);
      ar >> *reinterpret_cast<SetWorkOrchestratorProcessPolicyTask*>(task_ptr.task_);
      break;
    }
  }
  return task_ptr;
}
/** Serialize a task when returning from remote queue */
std::vector<DataTransfer> SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kRegisterTaskLib: {
      ar << *reinterpret_cast<RegisterTaskLibTask*>(task);
      break;
    }
    case Method::kDestroyTaskLib: {
      ar << *reinterpret_cast<DestroyTaskLibTask*>(task);
      break;
    }
    case Method::kCreateTaskState: {
      ar << *reinterpret_cast<CreateTaskStateTask*>(task);
      break;
    }
    case Method::kGetTaskStateId: {
      ar << *reinterpret_cast<GetTaskStateIdTask*>(task);
      break;
    }
    case Method::kDestroyTaskState: {
      ar << *reinterpret_cast<DestroyTaskStateTask*>(task);
      break;
    }
    case Method::kStopRuntime: {
      ar << *reinterpret_cast<StopRuntimeTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorQueuePolicy: {
      ar << *reinterpret_cast<SetWorkOrchestratorQueuePolicyTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorProcessPolicy: {
      ar << *reinterpret_cast<SetWorkOrchestratorProcessPolicyTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when returning from remote queue */
void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kRegisterTaskLib: {
      ar >> *reinterpret_cast<RegisterTaskLibTask*>(task);
      break;
    }
    case Method::kDestroyTaskLib: {
      ar >> *reinterpret_cast<DestroyTaskLibTask*>(task);
      break;
    }
    case Method::kCreateTaskState: {
      ar >> *reinterpret_cast<CreateTaskStateTask*>(task);
      break;
    }
    case Method::kGetTaskStateId: {
      ar >> *reinterpret_cast<GetTaskStateIdTask*>(task);
      break;
    }
    case Method::kDestroyTaskState: {
      ar >> *reinterpret_cast<DestroyTaskStateTask*>(task);
      break;
    }
    case Method::kStopRuntime: {
      ar >> *reinterpret_cast<StopRuntimeTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorQueuePolicy: {
      ar >> *reinterpret_cast<SetWorkOrchestratorQueuePolicyTask*>(task);
      break;
    }
    case Method::kSetWorkOrchestratorProcessPolicy: {
      ar >> *reinterpret_cast<SetWorkOrchestratorProcessPolicyTask*>(task);
      break;
    }
  }
}

#endif  // LABSTOR_LABSTOR_ADMIN_METHODS_H_