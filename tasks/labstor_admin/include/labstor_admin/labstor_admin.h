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
  void RegisterTaskLibrary(const TaskNode &task_node,
                           const DomainId &domain_id,
                           const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<RegisterTaskLibTask>(
        p, task_node, domain_id, lib_name);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(RegisterTaskLibrary);

  /** Unregister a task */
  HSHM_ALWAYS_INLINE
  void DestroyTaskLibrary(const TaskNode &task_node,
                          const DomainId &domain_id,
                          const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTaskLibTask>(
        p, task_node, domain_id, lib_name);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(DestroyTaskLibrary);

  /** Asynchronously spawn a task state */
  template<typename CreateTaskStateT, typename ...Args>
  HSHM_ALWAYS_INLINE
  CreateTaskStateT* AsyncCreateTaskState(const TaskNode &task_node,
                                         Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<CreateTaskStateT>(p, task_node, std::forward<Args>(args)...);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncCreateTaskState);

  /** Spawn a task state */
  template<typename CreateTaskStateT, typename ...Args>
  HSHM_ALWAYS_INLINE
  TaskStateId CreateTaskState(const TaskNode &task_node,
                              Args&& ...args) {
    auto *task = AsyncCreateTaskState<CreateTaskStateT>(task_node, std::forward<Args>(args)...);
    task->Wait();
    TaskStateId new_id = task->id_;
    LABSTOR_CLIENT->DelTask(task);
    if (new_id.IsNull()) {
      HELOG(kWarning, "Failed to create task state");
    }
    return new_id;
  }
  LABSTOR_TASK_NODE_ROOT(CreateTaskState);

  /** Get the ID of a task state */
  TaskStateId GetTaskStateId(const TaskNode &task_node,
                             const DomainId &domain_id,
                             const std::string &state_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<GetTaskStateIdTask>(
        p, task_node, domain_id, state_name);
    queue->Emplace(0, p);
    task->Wait();
    TaskStateId new_id = task->id_;
    LABSTOR_CLIENT->DelTask(task);
    return new_id;
  }
  LABSTOR_TASK_NODE_ROOT(GetTaskStateId);

  /** Terminate a task state */
  HSHM_ALWAYS_INLINE
  void DestroyTaskState(const TaskNode &task_node,
                        const DomainId &domain_id,
                        const TaskStateId &id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTaskStateTask>(
        p, task_node, domain_id, id);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(DestroyTaskState);

  /** Terminate the runtime */
  void StopRuntime(const TaskNode &task_node,
                   const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<StopRuntimeTask>(
        p, task_node, domain_id);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(StopRuntime);

  /** Set work orchestrator queue policy */
  void SetWorkOrchestratorQueuePolicy(const TaskNode &task_node,
                                      const DomainId &domain_id,
                                      const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<SetWorkOrchestratorQueuePolicyTask>(
        p, task_node, domain_id, policy);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(SetWorkOrchestratorQueuePolicy);

  /** Set work orchestrator process policy */
  void SetWorkOrchestratorProcessPolicy(const TaskNode &task_node,
                                        const DomainId &domain_id,
                                        const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<SetWorkOrchestratorProcessPolicyTask>(
        p, task_node, domain_id, policy);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(SetWorkOrchestratorProcessPolicy);
};

}  // namespace labstor::Admin

#define LABSTOR_ADMIN \
  hshm::EasySingleton<labstor::Admin::Client>::GetInstance()

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
