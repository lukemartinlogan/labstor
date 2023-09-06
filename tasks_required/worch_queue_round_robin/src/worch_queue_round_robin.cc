//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "worch_queue_round_robin/worch_queue_round_robin.h"

namespace labstor::worch_queue_round_robin {

class Server : public TaskLib {
 public:
  u32 count_;

 public:
  void Construct(MultiQueue *queue, ConstructTask *task) {
    count_ = 0;
    task->SetModuleComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetModuleComplete();
  }

  void Schedule(MultiQueue *queue, ScheduleTask *task) {
    // Check if any new queues need to be scheduled
    for (MultiQueue &queue : *LABSTOR_QM_RUNTIME->queue_map_) {
      if (queue.id_.IsNull()) {
        continue;
      }
      if (count_ == 0) {
        // Admin queue is scheduled on the first worker
        HILOG(kDebug, "Scheduling the queue {}", queue.id_);
        Worker &worker = LABSTOR_WORK_ORCHESTRATOR->workers_[0];
        worker.PollQueues({WorkEntry(0, &queue)});
        queue.num_scheduled_ = 1;
        count_ += 1;
        continue;
      }
      for (u32 lane_id = queue.num_scheduled_; lane_id < queue.num_lanes_; ++lane_id) {
        // NOTE(llogan): Assumes a minimum of two workers
        HILOG(kDebug, "Scheduling the queue {} (lane {})", queue.id_, lane_id);
        u32 worker_id = (count_ % (LABSTOR_WORK_ORCHESTRATOR->workers_.size() - 1)) + 1;
        Worker &worker = LABSTOR_WORK_ORCHESTRATOR->workers_[worker_id];
        worker.PollQueues({WorkEntry(lane_id, &queue)});
        count_ += 1;
      }
      queue.num_scheduled_ = queue.num_lanes_;
    }
  }

#include "worch_queue_round_robin/worch_queue_round_robin_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::worch_queue_round_robin::Server, "worch_queue_round_robin");
