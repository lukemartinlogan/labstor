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
  TASK_METHOD_T kCreateQueueAndId = TaskMethod::kLast + 1;
  TASK_METHOD_T kDestroyQueue = TaskMethod::kLast + 2;
};

/** A request used for communicating between clients & runtime */
struct CreateQueueTask : public Task {
  QueueId id_;
  u32 max_lanes_;
  u32 num_lanes_;
  u32 depth_;
  bitfield32_t flags_;

  HSHM_ALWAYS_INLINE
  CreateQueueTask(u32 node_id,
                  const QueueId &id,
                  u32 max_lanes, u32 num_lanes,
                  u32 depth, bitfield32_t flags) {
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

  HSHM_ALWAYS_INLINE
  CreateQueueTask(u32 node_id,
                  u32 max_lanes, u32 num_lanes,
                  u32 depth, bitfield32_t flags) {
    // Initialize task
    key_ = 0;
    task_exec_ = QueueManager::kAdminTaskExec;
    method_ = AdminMethod::kCreateQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    max_lanes_ = max_lanes;
    num_lanes_ = num_lanes;
    depth_ = depth;
    flags_ = flags;
  }
};

/** A request used for communicating between clients & runtime */
struct DestroyQueueTask : public Task {
  QueueId id_;

  HSHM_ALWAYS_INLINE
  DestroyQueueTask(u32 node_id,
                   const QueueId &id) {
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

/** Create admin requests */
class LabstorAdmin {
 public:
  /** Default constructor */
  LabstorAdmin() = default;

  /** Destructor */
  ~LabstorAdmin() = default;

  /** Create a queue */
  QueueId CreateQueue(u32 node_id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<CreateQueueTask>(LABSTOR_CLIENT->main_alloc_, p,
                                                  node_id,
                                                  max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Create a queue with an ID */
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
  void DestroyQueue(QueueId id, u32 node_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);
    auto *task = queue->Allocate<DestroyQueueTask>(LABSTOR_CLIENT->main_alloc_, p, node_id, id);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Register a task library */
  void RegisterTaskLibrary() {
  }

  /** Unregister a task */
  void UnregisterTaskLibrary() {
  }

  /** Spawn a task executor */
  void CreateTaskExecutor() {
  }

  /** Terminate a task executor */
  void DestroyTaskExecutor() {
  }
};

}  // namespace labstor

#endif //LABSTOR_TASKS_LABSTOR_ADMIN_LABSTOR_ADMIN_H_
