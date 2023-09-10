//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_

#include "labstor_admin_tasks.h"

namespace labstor::Admin {

/** Create admin requests */
class Client {
 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Register a task library */
  HSHM_ALWAYS_INLINE
  RegisterTaskLibTask* AsyncRegisterTaskLibrary(const TaskNode &task_node,
                                                const DomainId &domain_id,
                                                const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<RegisterTaskLibTask>(
        p, task_node, domain_id, lib_name);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncRegisterTaskLibrary);
  HSHM_ALWAYS_INLINE
  void RegisterTaskLibraryRoot(const DomainId &domain_id,
                               const std::string &lib_name) {
    RegisterTaskLibTask *task = AsyncRegisterTaskLibraryRoot(domain_id, lib_name);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Unregister a task */
  HSHM_ALWAYS_INLINE
  DestroyTaskLibTask* AsyncDestroyTaskLibrary(const TaskNode &task_node,
                                              const DomainId &domain_id,
                                              const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTaskLibTask>(
        p, task_node, domain_id, lib_name);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncDestroyTaskLibrary);
  HSHM_ALWAYS_INLINE
  void DestroyTaskLibraryRoot(const TaskNode &task_node,
                              const DomainId &domain_id,
                              const std::string &lib_name) {
    DestroyTaskLibTask *task = AsyncDestroyTaskLibraryRoot(domain_id, lib_name);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Spawn a task state */
  template<typename CreateTaskStateT, typename ...Args>
  HSHM_ALWAYS_INLINE
  CreateTaskStateT* AsyncCreateTaskState(const TaskNode &task_node,
                                         Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<CreateTaskStateT>(p, task_node, std::forward<Args>(args)...);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncCreateTaskState);
  template<typename CreateTaskStateT, typename ...Args>
  HSHM_ALWAYS_INLINE
  TaskStateId CreateTaskStateRoot(Args&& ...args) {
    auto *task = AsyncCreateTaskStateRoot<CreateTaskStateT>(std::forward<Args>(args)...);
    task->Wait();
    TaskStateId new_id = task->id_;
    LABSTOR_CLIENT->DelTask(task);
    if (new_id.IsNull()) {
      HELOG(kWarning, "Failed to create task state");
    }
    return new_id;
  }

  /** Get the ID of a task state */
  GetOrCreateTaskStateIdTask* AsyncGetOrCreateTaskStateId(const TaskNode &task_node,
                                                          const DomainId &domain_id,
                                                          const std::string &state_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<GetOrCreateTaskStateIdTask>(
        p, task_node, domain_id, state_name);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetOrCreateTaskStateId);
  TaskStateId GetOrCreateTaskStateIdRoot(const DomainId &domain_id,
                                         const std::string &state_name) {
    GetOrCreateTaskStateIdTask *task = AsyncGetOrCreateTaskStateIdRoot(domain_id, state_name);
    task->Wait();
    TaskStateId new_id = task->id_;
    LABSTOR_CLIENT->DelTask(task);
    return new_id;
  }

  /** Get the ID of a task state */
  GetTaskStateIdTask* AsyncGetTaskStateId(const TaskNode &task_node,
                                          const DomainId &domain_id,
                                          const std::string &state_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<GetTaskStateIdTask>(
        p, task_node, domain_id, state_name);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetTaskStateId);
  TaskStateId GetTaskStateIdRoot(const DomainId &domain_id,
                                 const std::string &state_name) {
    GetTaskStateIdTask *task = AsyncGetTaskStateIdRoot(domain_id, state_name);
    task->Wait();
    TaskStateId new_id = task->id_;
    LABSTOR_CLIENT->DelTask(task);
    return new_id;
  }

  /** Terminate a task state */
  HSHM_ALWAYS_INLINE
  DestroyTaskStateTask* AsyncDestroyTaskState(const TaskNode &task_node,
                                              const DomainId &domain_id,
                                              const TaskStateId &id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTaskStateTask>(
        p, task_node, domain_id, id);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncDestroyTaskState);
  HSHM_ALWAYS_INLINE
  void DestroyTaskStateRoot(const DomainId &domain_id,
                            const TaskStateId &id) {
    DestroyTaskStateTask *task = AsyncDestroyTaskStateRoot(domain_id, id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Terminate the runtime */
  void AsyncStopRuntime(const TaskNode &task_node,
                        const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    LABSTOR_CLIENT->NewTask<StopRuntimeTask>(
        p, task_node, domain_id);
    queue->Emplace(0, p);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncStopRuntime);

  /** Set work orchestrator queue policy */
  SetWorkOrchQueuePolicyTask*
  AsyncSetWorkOrchQueuePolicy(const TaskNode &task_node,
                              const DomainId &domain_id,
                              const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<SetWorkOrchQueuePolicyTask>(
        p, task_node, domain_id, policy);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncSetWorkOrchQueuePolicy);
  void SetWorkOrchQueuePolicyRoot(const DomainId &domain_id,
                                  const TaskStateId &policy) {
    SetWorkOrchQueuePolicyTask *task = AsyncSetWorkOrchQueuePolicyRoot(domain_id, policy);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Set work orchestrator process policy */
  SetWorkOrchProcPolicyTask*
  AsyncSetWorkOrchProcPolicy(const TaskNode &task_node,
                             const DomainId &domain_id,
                             const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<SetWorkOrchProcPolicyTask>(
        p, task_node, domain_id, policy);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncSetWorkOrchProcPolicy);
  void SetWorkOrchProcPolicyRoot(const DomainId &domain_id,
                                 const TaskStateId &policy) {
    SetWorkOrchProcPolicyTask *task = AsyncSetWorkOrchProcPolicyRoot(domain_id, policy);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
};

}  // namespace labstor::Admin

#define LABSTOR_ADMIN \
  hshm::EasySingleton<labstor::Admin::Client>::GetInstance()

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
