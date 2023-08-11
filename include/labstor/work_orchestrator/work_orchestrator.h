//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORK_ORCHESTRATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORK_ORCHESTRATOR_H_

#include "labstor/labstor_types.h"
#include "labstor/api/labstor_runtime.h"
#include "labstor/queue_manager/queue_manager_runtime.h"
#include "worker.h"
#include "labstor/network/rpc_thallium.h"
#include <thread>

namespace labstor {

class WorkOrchestrator {
 public:
  ServerConfig *config_;  /**< The server configuration */
  std::vector<Worker> workers_;  /**< Workers execute tasks */
  std::atomic<bool> stop_runtime_;  /**< Begin killing the runtime */
  std::atomic<bool> kill_requested_;  /**< Kill flushing threads eventually */
  ABT_xstream xstream_;

 public:
  /** Default constructor */
  WorkOrchestrator() = default;

  /** Destructor */
  ~WorkOrchestrator() = default;

  /** Create thread pool */
  void ServerInit(ServerConfig *config, QueueManager &qm) {
    config_ = config;

    // Create argobots xstream
    int ret = ABT_xstream_create(ABT_SCHED_NULL, &xstream_);
    if (ret != ABT_SUCCESS) {
      HELOG(kFatal, "Could not create argobots xstream");
    }

    // Spawn workers on the stream
    size_t num_workers = config_->wo_.max_workers_;
    workers_.reserve(num_workers);
    for (u32 worker_id = 0; worker_id < num_workers; ++worker_id) {
      workers_.emplace_back(worker_id, xstream_);
    }
    stop_runtime_ = false;
    kill_requested_ = false;

    // Initially round-robin admin queue across workers
    u32 count = 0;
    for (MultiQueue &queue : *qm.queue_map_) {
      for (u32 lane_id = 0; lane_id < queue.num_lanes_; ++lane_id) {
        u32 worker_id = count % workers_.size();
        Worker &worker = workers_[worker_id];
        worker.PollQueues({WorkEntry(lane_id, &queue)});
        count += 1;
      }
    }

    HILOG(kInfo, "Started {} workers", num_workers);
  }

  /** Get worker with this id */
  Worker& GetWorker(u32 worker_id) {
    return workers_[worker_id];
  }

  /** Get the number of workers */
  size_t GetNumWorkers() {
    return workers_.size();
  }

  /** Begin finalizing the runtime */
  void FinalizeRuntime() {
    stop_runtime_.store(true);
  }

  /** Finalize thread pool */
  void Join() {
    kill_requested_.store(true);
    for (Worker &worker : workers_) {
      // worker.thread_->join();
      ABT_xstream_join(xstream_);
      ABT_xstream_free(&xstream_);
    }
  }

  /** Whether threads should still be executing */
  bool IsAlive() {
    return !kill_requested_.load();
  }

  /** Whether runtime should still be executing */
  bool IsRuntimeAlive() {
    return !stop_runtime_.load();
  }
};

}  // namespace labstor

#define LABSTOR_WORK_ORCHESTRATOR \
  (&LABSTOR_RUNTIME->work_orchestrator_)

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORK_ORCHESTRATOR_H_
