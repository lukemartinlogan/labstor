//
// Created by lukemartinlogan on 6/28/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_

#include "labstor/config/config_server.h"
#include "labstor/labstor_types.h"
#include "queue.h"

namespace labstor {

/** Shared-memory representation of the QueueManager */
struct QueueManagerShm {
  hipc::ShmArchive<hipc::vector<MultiQueue>> queue_map_;
  hipc::ShmArchive<hipc::split_ticket_queue<size_t>> tickets_;
};

/** A base class inherited by Client & Server QueueManagers */
class QueueManager {
 public:
  hipc::vector<MultiQueue> *queue_map_;
  static inline const QueueId kAdmin = QueueId(0, 0);

 public:
  /**
   * Get a queue by ID
   *
   * TODO(llogan): Maybe make a local hashtable to map id -> ticket?
   * */
  HSHM_ALWAYS_INLINE MultiQueue* GetQueue(const QueueId &id) {
    return &(*queue_map_)[id.unique_];
  }
};

/** The set of methods in the admin task */
struct QueueManagerMethod : public TaskMethod {
  TASK_METHOD_T kCreateQueue = TaskMethod::kLast;
  TASK_METHOD_T kDestroyQueue = TaskMethod::kLast + 1;
};

/** A request used for communicating between clients & runtime */
struct CreateQueueTask : public Task {
  QueueId id_;
  u32 max_lanes_;
  u32 num_lanes_;
  u32 depth_;
  bitfield32_t flags_;

  HSHM_ALWAYS_INLINE
  CreateQueueTask(TaskExecId task_exec,
                  u32 node_id,
                  const QueueId &id,
                  u32 max_lanes, u32 num_lanes,
                  u32 depth, bitfield32_t flags) {
    // Initialize task
    key_ = 0;
    task_exec_ = task_exec;
    method_ = QueueManagerMethod::kCreateQueue;
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

/** A request used for communicating between clients & runtime */
struct DestroyQueueTask : public Task {
  QueueId id_;

  HSHM_ALWAYS_INLINE
  DestroyQueueTask(TaskExecId task_exec,
                   u32 node_id,
                   const QueueId &id) {
    // Initialize task
    key_ = 0;
    task_exec_ = task_exec;
    method_ = QueueManagerMethod::kCreateQueue;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Initialize QueueManager task
    id_ = id;
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_
