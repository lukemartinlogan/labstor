//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_

#include "labstor/api/labstor_client.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor {

/** The set of methods in the admin task */
struct AdminMethod : public TaskMethod {
  TASK_METHOD_T kCreateQueue = TaskMethod::kLast;
  TASK_METHOD_T kDestroyQueue = TaskMethod::kLast + 1;
  TASK_METHOD_T kRegisterTaskLib = TaskMethod::kLast + 2;
  TASK_METHOD_T kUnregisterTaskLib = TaskMethod::kLast + 3;
  TASK_METHOD_T kCreateTaskExecutor = TaskMethod::kLast + 4;
  TASK_METHOD_T kGetTaskExecutorId = TaskMethod::kLast + 5;
  TASK_METHOD_T kDestroyTaskExecutor = TaskMethod::kLast + 6;
};

/** A task to create a queue */
struct CreateQueueTask : public Task {
  INOUT QueueId id_;
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
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kCreateQueue;
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
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    id_ = id;
  }
};

/** A task to register a Task Library */
struct RegisterTaskLibTask : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  OUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  RegisterTaskLibTask(hipc::Allocator *alloc,
                      u32 node_id,
                      const std::string &lib_name) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
  }

  ~RegisterTaskLibTask() {
    HSHM_DESTROY_AR(lib_name_);
  }
};

/** A task to destroy a Task Library */
typedef RegisterTaskLibTask DestroyTaskLibTask;

/** A task to register a Task Executor */
struct CreateTaskExecutorTask : public Task {
  IN hipc::ShmArchive<hipc::string> exec_name_;
  INOUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  CreateTaskExecutorTask(hipc::Allocator *alloc,
                         u32 node_id,
                         const std::string &exec_name,
                         const TaskExecId &id = TaskExecId::GetNull()) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(exec_name_, alloc, exec_name);
    id_ = id;
  }

  ~CreateTaskExecutorTask() {
    HSHM_DESTROY_AR(exec_name_);
  }
};

/** A task to retrieve the ID of a task */
struct GetTaskExecutorIdTask : public Task {
  IN hipc::ShmArchive<hipc::string> exec_name_;
  OUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  GetTaskExecutorIdTask(hipc::Allocator *alloc, u32 node_id,
                        const std::string &exec_name) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(exec_name_, alloc, exec_name);
  }

  ~GetTaskExecutorIdTask() {
    HSHM_DESTROY_AR(exec_name_);
  }
};

/** A task to destroy a Task Executor */
struct DestroyTaskExecutorTask : public Task {
  IN TaskExecId id_;

  HSHM_ALWAYS_INLINE
  DestroyTaskExecutorTask(hipc::Allocator *alloc,
                          TaskExecId id,
                          u32 node_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    id_ = id;
  }
};

/** Create admin requests */
class LabstorAdmin {
 public:
  /** Default constructor */
  LabstorAdmin() = default;

  /** Destructor */
  ~LabstorAdmin() = default;

  /** Create a queue */
  HSHM_ALWAYS_INLINE
  void CreateQueue(u32 node_id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateQueueAndIdTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                       node_id,
                                                       max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Create a queue with an ID */
  HSHM_ALWAYS_INLINE
  void CreateQueue(QueueId id, u32 node_id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateQueueTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                  node_id, id,
                                                  max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Remove a queue */
  HSHM_ALWAYS_INLINE
  void DestroyQueue(QueueId id, u32 node_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyQueueTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, id);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Register a task library */
  template<typename T, typename ...Args>
  HSHM_ALWAYS_INLINE
  void RegisterTaskLibrary(u32 node_id, Args&& ...args) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<RegisterTaskLibTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                      node_id, std::forward<Args>(args)...);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Unregister a task */
  HSHM_ALWAYS_INLINE
  void DestroyTaskLibrary(u32 node_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyTaskLibTask>(LABSTOR_CLIENT->main_alloc_, p, node_id);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Spawn a task executor */
  HSHM_ALWAYS_INLINE
  void CreateTaskExecutor() {
  }

  /** Terminate a task executor */
  HSHM_ALWAYS_INLINE
  void DestroyTaskExecutor() {
  }
};

}  // namespace labstor

#endif //LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
