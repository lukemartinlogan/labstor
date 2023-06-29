//
// Created by lukemartinlogan on 6/28/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_CLIENT_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_CLIENT_H_

#include "queue_manager.h"

namespace labstor {

#define LABSTOR_QUEUE_MANAGER_CLIENT \
  (&LABSTOR_CLIENT->queue_manager_)

/** Enable client programs to access queues */
class QueueManagerClient : public QueueManager {
 public:
  u32 node_id_;
  hipc::Allocator *alloc_;

 public:
  /** Default constructor */
  QueueManagerClient() = default;

  /** Destructor*/
  ~QueueManagerClient() = default;

  /** Initialize client */
  void ClientInit(hipc::Allocator *alloc, QueueManagerShm &shm) {
    alloc_ = alloc;
    queue_map_ = shm.queue_map_.get();
  }

  /** Create a queue */
  QueueId CreateQueue(u32 node_id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    hipc::Pointer p;
    // Send request to server
    MultiQueue *queue = GetQueue(kAdmin);
    queue->Allocate<CreateQueueTask>(alloc_, p,
                                     kAdmin, node_id,
                                     max_lanes, num_lanes, depth, flags);
    queue->Emplace(0, task.p_);
    // Wait for response
    // Create queue in local memory
  }

  /** Create a queue with an ID */
  void CreateQueue(QueueId id, u32 max_lanes, u32 num_lanes, u32 depth, bitfield32_t flags) {
    // Send request to server
    // Wait for response
    // Create queue in local memory
  }

  /** Remove a queue */
  void RemoveQueue(QueueId id) {
    // Send request to server
    // Wait for response
    // Remove queue from local memory
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_CLIENT_H_
