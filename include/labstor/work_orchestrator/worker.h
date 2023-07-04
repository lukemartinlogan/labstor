//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_

#include "labstor/labstor_types.h"
#include "hermes_shm/data_structures/ipc/mpsc_queue.h"

namespace labstor {

typedef std::pair<u32, MultiQueue*> WorkEntry;

class Worker {
 public:
  u32 id_;
  u32 numa_node_;  // TODO(llogan): track NUMA affinity
  std::vector<WorkEntry> work_queue_;
  hshm::spsc_queue<std::vector<WorkEntry>> poll_queues_;
  hshm::spsc_queue<std::vector<WorkEntry>> relinquish_queues_;

 public:
  /** Constructor */
  Worker(u32 id) {
    id_ = id;
  }

  /**
   * Tell worker to poll a set of queues
   * */
  void PollQueues(const std::vector<WorkEntry> &queues) {
    poll_queues_.emplace(queues);
  }

  /**
   * Tell worker to start relinquishing some of its queues
   * This function must be called from a single thread (outside of worker)
   * */
  void RelinquishingQueues(const std::vector<WorkEntry> &queues) {
    relinquish_queues_.emplace(queues);
  }

  /** Check if worker is still stealing queues */
  bool IsRelinquishingQueues() {
    return relinquish_queues_.size() > 0;
  }

  /** The work loop */
  void Loop();

 private:
  /** Worker merges the set of queues to poll */
  void _PollQueues() {
    std::vector<WorkEntry> work_queue;
    while (!poll_queues_.pop(work_queue).IsNull()) {
      for (auto &entry : work_queue) {
        work_queue_.emplace_back(entry);
      }
    }
  }

  /** Relinquish queues. Called from wihtin Worker */
  void _RelinquishQueues() {
    std::vector<WorkEntry> work_queue;
    while (!poll_queues_.pop(work_queue).IsNull()) {
      for (auto &entry : work_queue) {
        work_queue_.erase(std::find(work_queue_.begin(),
                                    work_queue_.end(), entry));
      }
    }
  }

  /** Poll work queue */
  void Run();
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
