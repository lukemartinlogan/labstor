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
                  const TaskNode &task_node,
                  const DomainId &domain_id,
                  const QueueId &id,
                  u32 max_lanes, u32 num_lanes,
                  u32 depth, bitfield32_t flags) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kCreateQueue;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

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
                   const TaskNode &task_node,
                   const DomainId &domain_id,
                   const QueueId &id): Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kDestroyQueue;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

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
                           const TaskNode &task_node,
                           const DomainId &domain_id,
                           const std::string &lib_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    if constexpr(method == 0) {
      method_ = Method::kRegisterTaskLib;
    } else {
      method_ = Method::kDestroyTaskLib;
    }
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

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

class CreateTaskStatePhase {
 public:
  // NOTE(llogan): kLast is intentially 0 so that the constructor
  // can seamlessly pass data to submethods
  TASK_METHOD_T kLast = 0;
};

/** A task to register a Task state */
struct CreateTaskStateTask : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  IN hipc::ShmArchive<hipc::string> state_name_;
  INOUT TaskStateId id_;
  OUT int phase_;

  HSHM_ALWAYS_INLINE
  CreateTaskStateTask(hipc::Allocator *alloc,
                      const TaskNode &task_node,
                      const DomainId &domain_id,
                      const std::string &state_name,
                      const std::string &lib_name,
                      const TaskStateId &id = TaskStateId::GetNull()) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kCreateTaskState;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

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
                     const TaskNode &task_node,
                     const DomainId &domain_id,
                     const std::string &state_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kGetTaskStateId;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

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
                       const TaskNode &task_node,
                       const DomainId &domain_id,
                       const TaskStateId &id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kDestroyTaskState;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize
    id_ = id;
  }
};

/** A task to destroy a Task state */
struct StopRuntimeTask : public Task {
  HSHM_ALWAYS_INLINE
  StopRuntimeTask(hipc::Allocator *alloc,
                  const TaskNode &task_node,
                  const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kStopRuntime;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }
};

/** A task to destroy a Task state */
template<int method>
struct SetWorkOrchestratorPolicyTask : public Task {
  IN TaskStateId policy_id_;

  HSHM_ALWAYS_INLINE
  SetWorkOrchestratorPolicyTask(hipc::Allocator *alloc,
                                const TaskNode &task_node,
                                const DomainId &domain_id,
                                const TaskStateId &policy_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    if constexpr(method == 0) {
      method_ = Method::kSetWorkOrchestratorQueuePolicy;
    } else {
      method_ = Method::kSetWorkOrchestratorProcessPolicy;
    }
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize
    policy_id_ = policy_id;
  }
};
using SetWorkOrchestratorQueuePolicyTask = SetWorkOrchestratorPolicyTask<0>;
using SetWorkOrchestratorProcessPolicyTask = SetWorkOrchestratorPolicyTask<1>;


/** Stores a CreateQueue + CreateTaskState task */
struct CreateTaskStateInfo {
  CreateQueueTask *queue_task_;
  CreateTaskStateTask *state_task_;
};

/** Create admin requests */
class Client {
 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Async create a queue with an ID */
  HSHM_ALWAYS_INLINE
  CreateQueueTask* ACreateQueue(const TaskNode &task_node,
                                const DomainId &domain_id,
                                const QueueId &id,
                                u32 max_lanes, u32 num_lanes,
                                u32 depth, bitfield32_t flags) {
    if (id.IsNull()) {
      HELOG(kWarning, "Cannot create a queue with a null ID");
      return nullptr;
    }
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<CreateQueueTask>(
        p, task_node, domain_id, id,
        max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(ACreateQueue);

  /** Create a queue with an ID */
  HSHM_ALWAYS_INLINE
  void CreateQueue(const TaskNode &task_node,
                   const DomainId &domain_id,
                   const QueueId &id,
                   u32 max_lanes, u32 num_lanes,
                   u32 depth, bitfield32_t flags) {
    auto task = ACreateQueue(task_node, domain_id, id, max_lanes, num_lanes, depth, flags);
    if (!task) {
      return;
    }
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(CreateQueue);

  /** Remove a queue */
  HSHM_ALWAYS_INLINE
  void DestroyQueue(const TaskNode &task_node,
                    const DomainId &domain_id,
                    const QueueId &id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<DestroyQueueTask>(p, task_node, domain_id, id);
    queue->Emplace(0, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(DestroyQueue);

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
  CreateTaskStateT* ACreateTaskState(const TaskNode &task_node,
                                     Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<CreateTaskStateT>(p, task_node, std::forward<Args>(args)...);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(ACreateTaskState);

  /** Spawn a task state */
  template<typename CreateTaskStateT, typename ...Args>
  HSHM_ALWAYS_INLINE
  TaskStateId CreateTaskState(const TaskNode &task_node,
                              Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(LABSTOR_QM_CLIENT->admin_queue_);
    auto *task = LABSTOR_CLIENT->NewTask<CreateTaskStateT>(
        p, task_node, std::forward<Args>(args)...);
    queue->Emplace(0, p);
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
