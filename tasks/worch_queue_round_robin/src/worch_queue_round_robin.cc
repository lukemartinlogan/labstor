//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "worch_queue_round_robin/worch_queue_round_robin.h"

namespace labstor::worch_queue_round_robin {

class Server : public TaskLib {
 public:
  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kSchedule: {
        Schedule(queue, task);
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
  }

  void Schedule(MultiQueue *queue, Task *task) {
    // Check if any new queues need to be scheduled
    u32 count = 0;
    for (MultiQueue &queue : *LABSTOR_QM_RUNTIME->queue_map_) {
      for (u32 lane_id = 0; lane_id < queue.num_lanes_; ++lane_id) {
        u32 worker_id = count % LABSTOR_WORK_ORCHESTRATOR->workers_.size();
        Worker &worker = LABSTOR_WORK_ORCHESTRATOR->workers_[worker_id].worker_;
        worker.PollQueues({WorkEntry(lane_id, &queue)});
      }
    }

    // Check if queues need to be reshuffled
    for (WorkerInfo &work_info : LABSTOR_WORK_ORCHESTRATOR->workers_) {
      Worker &worker = work_info.worker_;
      // Iterate over queues in queue manager
      for (WorkEntry &work_entry : worker.work_queue_) {
        u32 lane = work_entry.first;
        MultiQueue *queue = work_entry.second;
      }
    }
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::worch_queue_round_robin::Server, "worch_queue_round_robin");
