//
// Created by lukemartinlogan on 6/28/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_SERVER_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_SERVER_H_

#include "queue_manager.h"

namespace labstor {

#define LABSTOR_QM_RUNTIME \
  (&LABSTOR_RUNTIME->queue_manager_)

/** Administrative queuing actions */
class QueueManagerRuntime : public QueueManager {
 public:
  ServerConfig *config_;
  size_t max_queues_;
  hipc::split_ticket_queue<u64> *tickets_;
  hipc::vector<MultiQueue> *queue_map_;
  u32 node_id_;

 public:
  /** Default constructor */
  QueueManagerRuntime() = default;

  /** Destructor*/
  ~QueueManagerRuntime() = default;

  /** Create queues in shared memory */
  void ServerInit(hipc::Allocator *alloc, int node_id, ServerConfig *config, QueueManagerShm &shm) {
    config_ = config;
    node_id_ = node_id;
    auto &qm_conf = config_->queue_manager_;
    // Initialize ticket queue (ticket 0 is for admin queue)
    max_queues_ = qm_conf.max_queues_;
    HSHM_MAKE_AR(shm.tickets_, alloc, max_queues_)
    for (u64 i = 1; i <= max_queues_; ++i) {
      shm.tickets_->emplace(i);
    }
    // Initialize queue map
    HSHM_MAKE_AR0(shm.queue_map_, alloc)
    queue_map_ = shm.queue_map_.get();
    queue_map_->reserve(max_queues_);
    // Create the admin queue
    CreateQueue(kAdminQueue,
                qm_conf.max_lanes_,
                qm_conf.max_lanes_,
                qm_conf.queue_depth_,
                bitfield32_t(TASK_LOW_LATENCY));
  }

  /** Create a new queue (with pre-allocated ID) in the map */
  void CreateQueue(const QueueId &id,
                   u32 max_lanes, u32 num_lanes,
                   u32 depth, bitfield32_t flags) {
    queue_map_->emplace(queue_map_->begin() + id.unique_,
                        id, max_lanes, num_lanes, depth, flags);
  }

  /**
   * Remove a queue
   *
   * For now, this function assumes that the queue is not in use.
   * TODO(llogan): don't assume this
   * */
  void DestroyQueue(QueueId &id) {
    queue_map_->erase(queue_map_->begin() + id.unique_);
    tickets_->emplace(id.unique_);
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_MANAGER_SERVER_H_
