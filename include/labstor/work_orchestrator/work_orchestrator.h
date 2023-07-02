//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORK_ORCHESTRATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORK_ORCHESTRATOR_H_

#include "labstor/labstor_types.h"
#include "labstor/api/labstor_runtime.h"
#include "worker.h"
#include <thread>

namespace labstor {

struct WorkerInfo {
  Worker worker_;
  std::thread thread_;

  /** Emplace constructor */
  explicit WorkerInfo(u32 id)
  : worker_(id), thread_(&Worker::Loop, &worker_) {}
};

class WorkOrchestrator {
 public:
  ServerConfig *config_;  /**< The server configuration */
  std::vector<WorkerInfo> workers_;  /**< Workers execute tasks */
  std::atomic<bool> stop_runtime_;  /**< Begin killing the runtime */
  std::atomic<bool> kill_requested_;  /**< Kill flushing threads eventually */

 public:
  /** Default constructor */
  WorkOrchestrator() = default;

  /** Destructor */
  ~WorkOrchestrator() = default;

  /** Create thread pool */
  void ServerInit(ServerConfig *config) {
    config_ = config;
    size_t num_workers = config_->wo_.max_workers_;
    workers_.reserve(num_workers);
    for (u32 worker_id = 0; worker_id < num_workers; ++worker_id) {
      workers_.emplace_back(worker_id);
    }
    stop_runtime_ = false;
    kill_requested_ = false;
  }

  /** Set CPU affinity of worker */
  void SetAffinity(u32 worker_id, u32 cpu) {
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpu, &cpuSet);
    auto &thread = workers_[worker_id].thread_;
    pthread_t threadId = thread.native_handle();
    int result = pthread_setaffinity_np(threadId,
                                        sizeof(cpu_set_t), &cpuSet);
    if (result != 0) {
      std::cerr << "Failed to set CPU affinity: " << result << std::endl;
    }
  }

  /** Get worker with this id */
  Worker& GetWorker(u32 worker_id) {
    return workers_[worker_id].worker_;
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
    for (WorkerInfo &worker : workers_) {
      worker.thread_.join();
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
