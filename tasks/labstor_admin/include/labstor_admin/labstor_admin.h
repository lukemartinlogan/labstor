//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_

#include "labstor/work_orchestrator/scheduler.h"
#include "labstor/api/labstor_client.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::Admin {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCreateQueue = TaskMethod::kLast;
  TASK_METHOD_T kDestroyQueue = TaskMethod::kLast + 1;
  TASK_METHOD_T kRegisterTaskLib = TaskMethod::kLast + 2;
  TASK_METHOD_T kDestroyTaskLib = TaskMethod::kLast + 3;
  TASK_METHOD_T kCreateTaskState = TaskMethod::kLast + 4;
  TASK_METHOD_T kGetTaskStateId = TaskMethod::kLast + 5;
  TASK_METHOD_T kDestroyTaskState = TaskMethod::kLast + 6;
  TASK_METHOD_T kStopRuntime = TaskMethod::kLast + 7;
  TASK_METHOD_T kSetWorkOrchestratorQueuePolicy = TaskMethod::kLast + 8;
  TASK_METHOD_T kSetWorkOrchestratorProcessPolicy = TaskMethod::kLast + 9;
  TASK_METHOD_T kWorkerPollQueue = TaskMethod::kLast + 10;
  TASK_METHOD_T kWorkerRelinquishQueue = TaskMethod::kLast + 11;
};

/** A task to create a queue */
struct CreateQueueTask : public Task {
  IN QueueId id_;
  IN u32 max_lanes_;
  IN u32 num_lanes_;
  IN u32 depth_;
  IN bitfield32_t flags_;

  HSHM_ALWAYS_INLINE
  CreateQueueTask(hipc::Allocator *alloc,
                  u32 node_id,
                  const QueueId &id,
                  u32 max_lanes, u32 num_lanes,
                  u32 depth, bitfield32_t flags) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kCreateQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    id_ = id;
    max_lanes_ = max_lanes;
    num_lanes_ = num_lanes;
    depth_ = depth;
    flags_ = flags;
  }
};

/** A task to destroy a queue */
struct DestroyQueueTask : public Task {
  IN QueueId id_;

  HSHM_ALWAYS_INLINE
  DestroyQueueTask(hipc::Allocator *alloc,
                   u32 node_id,
                   const QueueId &id): Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    id_ = id;
  }
};

/** A template to register or destroy a task library */
template<int method>
struct RegisterTaskLibTaskTempl : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  OUT TaskStateId id_;

  HSHM_ALWAYS_INLINE
  RegisterTaskLibTaskTempl(hipc::Allocator *alloc,
                           u32 node_id,
                           const std::string &lib_name) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    if constexpr(method == 0) {
      method_ = Method::kRegisterTaskLib;
    } else {
      method_ = Method::kDestroyTaskLib;
    }
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
  }

  ~RegisterTaskLibTaskTempl() {
    HSHM_DESTROY_AR(lib_name_);
  }
};

/** A task to register a Task Library */
using RegisterTaskLibTask = RegisterTaskLibTaskTempl<0>;

/** A task to destroy a Task Library */
using DestroyTaskLibTask = RegisterTaskLibTaskTempl<1>;

/** A task to register a Task state */
struct CreateTaskStateTask : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  IN hipc::ShmArchive<hipc::string> state_name_;
  INOUT TaskStateId id_;

  HSHM_ALWAYS_INLINE
  CreateTaskStateTask(hipc::Allocator *alloc,
                      u32 node_id,
                      const std::string &state_name,
                      const std::string &lib_name,
                      const TaskStateId &id = TaskStateId::GetNull()) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kCreateTaskState;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(state_name_, alloc, state_name);
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
    id_ = id;
  }

  ~CreateTaskStateTask() {
    HSHM_DESTROY_AR(state_name_);
    HSHM_DESTROY_AR(lib_name_);
  }
};

/** A task to retrieve the ID of a task */
struct GetTaskStateIdTask : public Task {
  IN hipc::ShmArchive<hipc::string> state_name_;
  OUT TaskStateId id_;

  HSHM_ALWAYS_INLINE
  GetTaskStateIdTask(hipc::Allocator *alloc,
                     u32 node_id,
                     const std::string &state_name) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kGetTaskStateId;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(state_name_, alloc, state_name);
  }

  ~GetTaskStateIdTask() {
    HSHM_DESTROY_AR(state_name_);
  }
};

/** A task to destroy a Task state */
struct DestroyTaskStateTask : public Task {
  IN TaskStateId id_;

  HSHM_ALWAYS_INLINE
  DestroyTaskStateTask(hipc::Allocator *alloc,
                       u32 node_id,
                       const TaskStateId &id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kDestroyTaskState;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    id_ = id;
  }
};

/** A task to destroy a Task state */
struct StopRuntimeTask : public Task {
  HSHM_ALWAYS_INLINE
  StopRuntimeTask(hipc::Allocator *alloc,
                  u32 node_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    method_ = Method::kStopRuntime;
    task_flags_.SetBits(0);
    node_id_ = node_id;
  }
};

/** A task to destroy a Task state */
template<int method>
struct SetWorkOrchestratorPolicyTask : public Task {
  IN TaskStateId policy_id_;

  HSHM_ALWAYS_INLINE
  SetWorkOrchestratorPolicyTask(hipc::Allocator *alloc,
                                u32 node_id,
                                const TaskStateId &policy_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = QueueManager::kAdminTaskState;
    if constexpr(method == 0) {
      method_ = Method::kSetWorkOrchestratorQueuePolicy;
    } else {
      method_ = Method::kSetWorkOrchestratorProcessPolicy;
    }
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    policy_id_ = policy_id;
  }
};
using SetWorkOrchestratorQueuePolicyTask = SetWorkOrchestratorPolicyTask<0>;
using SetWorkOrchestratorProcessPolicyTask = SetWorkOrchestratorPolicyTask<1>;


/** Create admin requests */
class Client {
 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a queue with an ID */
  HSHM_ALWAYS_INLINE
  void CreateQueue(u32 node_id, QueueId id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateQueueTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                  node_id, id,
                                                  max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Remove a queue */
  HSHM_ALWAYS_INLINE
  void DestroyQueue(u32 node_id, QueueId id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyQueueTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, id);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Register a task library */
  template<typename RegisterTaskT, typename ...Args>
  HSHM_ALWAYS_INLINE
  void RegisterTaskLibrary(u32 node_id, const std::string &lib_name, Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<RegisterTaskT>(LABSTOR_CLIENT->main_alloc_, p,
                                                node_id, lib_name, std::forward<Args>(args)...);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Unregister a task */
  HSHM_ALWAYS_INLINE
  void DestroyTaskLibrary(u32 node_id, const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyTaskLibTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, lib_name);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Spawn a task state */
  HSHM_ALWAYS_INLINE
      TaskStateId CreateTaskState(u32 node_id,
  const std::string &state_name,
  const std::string &lib_name,
  const TaskStateId &id = TaskStateId::GetNull()) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateTaskStateTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                      node_id, state_name, lib_name, id);
    queue->Emplace(0, p);
    task->Wait();
    TaskStateId new_id = task->id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return new_id;
  }

  /** Get the ID of a task state */
  TaskStateId GetTaskStateId(u32 node_id, const std::string &state_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<GetTaskStateIdTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, state_name);
    queue->Emplace(0, p);
    task->Wait();
    TaskStateId new_id = task->id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return new_id;
  }

  /** Terminate a task state */
  HSHM_ALWAYS_INLINE
  void DestroyTaskState(u32 node_id, const TaskStateId &id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyTaskStateTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, id);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Terminate the runtime */
  void StopRuntime(u32 node_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<StopRuntimeTask>(LABSTOR_CLIENT->main_alloc_, p, node_id);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Set work orchestrator queue policy */
  void SetWorkOrchestratorQueuePolicy(u32 node_id, const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<SetWorkOrchestratorQueuePolicyTask>(
        LABSTOR_CLIENT->main_alloc_, p, node_id, policy);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Set work orchestrator process policy */
  void SetWorkOrchestratorProcessPolicy(u32 node_id, const TaskStateId &policy) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<SetWorkOrchestratorProcessPolicyTask>(
        LABSTOR_CLIENT->main_alloc_, p, node_id, policy);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }
};

}  // namespace labstor::Admin

#define LABSTOR_ADMIN \
  hshm::EasySingleton<labstor::Admin::Client>::GetInstance()

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
