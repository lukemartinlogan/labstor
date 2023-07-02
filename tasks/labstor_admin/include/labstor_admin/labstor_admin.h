//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_

#include "labstor/api/labstor_client.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::Admin {

/** The set of methods in the admin task_templ */
struct Method : public TaskMethod {
  TASK_METHOD_T kCreateQueue = TaskMethod::kLast;
  TASK_METHOD_T kDestroyQueue = TaskMethod::kLast + 1;
  TASK_METHOD_T kRegisterTaskLib = TaskMethod::kLast + 2;
  TASK_METHOD_T kDestroyTaskLib = TaskMethod::kLast + 3;
  TASK_METHOD_T kCreateTaskExecutor = TaskMethod::kLast + 4;
  TASK_METHOD_T kGetTaskExecutorId = TaskMethod::kLast + 5;
  TASK_METHOD_T kDestroyTaskExecutor = TaskMethod::kLast + 6;
};

/** A task_templ to create a queue */
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
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = Method::kCreateQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task_templ
    id_ = id;
    max_lanes_ = max_lanes;
    num_lanes_ = num_lanes;
    depth_ = depth;
    flags_ = flags;
  }
};

/** A task_templ to destroy a queue */
struct DestroyQueueTask : public Task {
  IN QueueId id_;

  HSHM_ALWAYS_INLINE
  DestroyQueueTask(hipc::Allocator *alloc,
                   u32 node_id,
                   const QueueId &id): Task(alloc) {
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = Method::kDestroyQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task_templ
    id_ = id;
  }
};

/** A template to register or destroy a task_templ library */
template<int method>
struct RegisterTaskLibTaskTempl : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  OUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  RegisterTaskLibTaskTempl(hipc::Allocator *alloc,
                           u32 node_id,
                           const std::string &lib_name) : Task(alloc) {
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    if constexpr(method == 0) {
      method_ = Method::kRegisterTaskLib;
    } else {
      method_ = Method::kDestroyTaskLib;
    }
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task_templ
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
  }

  ~RegisterTaskLibTaskTempl() {
    HSHM_DESTROY_AR(lib_name_);
  }
};

/** A task_templ to register a Task Library */
using RegisterTaskLibTask = RegisterTaskLibTaskTempl<0>;

/** A task_templ to destroy a Task Library */
using DestroyTaskLibTask = RegisterTaskLibTaskTempl<1>;

/** A task_templ to register a Task Executor */
struct CreateTaskExecutorTask : public Task {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  IN hipc::ShmArchive<hipc::string> exec_name_;
  INOUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  CreateTaskExecutorTask(hipc::Allocator *alloc,
                         u32 node_id,
                         const std::string &exec_name,
                         const std::string &lib_name,
                         const TaskExecId &id = TaskExecId::GetNull()) : Task(alloc) {
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = Method::kCreateTaskExecutor;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(exec_name_, alloc, exec_name);
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
    id_ = id;
  }

  ~CreateTaskExecutorTask() {
    HSHM_DESTROY_AR(exec_name_);
    HSHM_DESTROY_AR(lib_name_);
  }
};

/** A task_templ to retrieve the ID of a task_templ */
struct GetTaskExecutorIdTask : public Task {
  IN hipc::ShmArchive<hipc::string> exec_name_;
  OUT TaskExecId id_;

  HSHM_ALWAYS_INLINE
  GetTaskExecutorIdTask(hipc::Allocator *alloc, u32 node_id,
                        const std::string &exec_name) : Task(alloc) {
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = Method::kGetTaskExecutorId;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    HSHM_MAKE_AR(exec_name_, alloc, exec_name);
  }

  ~GetTaskExecutorIdTask() {
    HSHM_DESTROY_AR(exec_name_);
  }
};

/** A task_templ to destroy a Task Executor */
struct DestroyTaskExecutorTask : public Task {
  IN TaskExecId id_;

  HSHM_ALWAYS_INLINE
  DestroyTaskExecutorTask(hipc::Allocator *alloc,
                          const TaskExecId &id,
                          u32 node_id) : Task(alloc) {
    // Initialize task_templ
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = Method::kDestroyTaskExecutor;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize
    id_ = id;
  }
};

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

  /** Register a task_templ library */
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

  /** Unregister a task_templ */
  HSHM_ALWAYS_INLINE
  void DestroyTaskLibrary(u32 node_id, const std::string &lib_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyTaskLibTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, lib_name);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Spawn a task_templ executor */
  HSHM_ALWAYS_INLINE
  TaskExecId CreateTaskExecutor(u32 node_id,
                          const std::string &exec_name,
                          const std::string &lib_name,
                          const TaskExecId &id = TaskExecId::GetNull()) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateTaskExecutorTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                         node_id, exec_name, lib_name, id);
    queue->Emplace(0, p);
    task->Wait();
    TaskExecId new_id = task->id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return new_id;
  }

  /** Get the ID of a task_templ executor */
  TaskExecId GetTaskExecutorId(u32 node_id, const std::string &exec_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<GetTaskExecutorIdTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, exec_name);
    queue->Emplace(0, p);
    task->Wait();
    TaskExecId new_id = task->id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return new_id;
  }

  /** Terminate a task_templ executor */
  HSHM_ALWAYS_INLINE
  void DestroyTaskExecutor(u32 node_id, const TaskExecId &id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyTaskExecutorTask>(LABSTOR_CLIENT->main_alloc_, p, id, node_id);
    queue->Emplace(0, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }
};

}  // namespace labstor::Admin

#define LABSTOR_ADMIN \
  hshm::EasySingleton<labstor::Admin::Client>::GetInstance()

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
