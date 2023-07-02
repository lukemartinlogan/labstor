//
// Created by lukemartinlogan on 6/28/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_

#include "labstor/config/config_server.h"
#include "labstor/labstor_types.h"
#include "queue_factory.h"

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
  static inline const QueueId kAdminQueue = QueueId(0, 0);
  static inline const TaskExecId kAdminTaskExec = TaskExecId(0, 0);

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

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_H_
