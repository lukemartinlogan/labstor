//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_

#include "labstor/labstor_types.h"
#include "labstor/queue_manager/queue_manager_runtime.h"
#include <thread>
#include "affinity.h"

namespace labstor {

#define WORKER_CONTINUOUS_POLLING (1 << 0)

typedef std::pair<u32, MultiQueue*> WorkEntry;

class Worker {
 public:
  u32 id_;  /**< Unique identifier of this worker */
  std::thread thread_;  /**< The worker thread handle */
  int pthread_id_;      /**< The worker pthread handle */
  int pid_;             /**< The worker process id */
  u32 numa_node_;       // TODO(llogan): track NUMA affinity
  std::vector<WorkEntry> work_queue_;  /**< The set of queues to poll */
  /** A set of queues to begin polling in a worker */
  hshm::spsc_queue<std::vector<WorkEntry>> poll_queues_;
  /** A set of queues to stop polling in a worker */
  hshm::spsc_queue<std::vector<WorkEntry>> relinquish_queues_;
  size_t sleep_us_;  /** Time the worker should sleep after a run */
  u32 retries_;      /** The number of times to repeat the internal run loop before sleeping */
  bitfield32_t flags_;  /** Worker metadata flags */

 public:
  /** Constructor */
  Worker(u32 id) : thread_(&Worker::Loop, this) {
    poll_queues_.Resize(256);
    relinquish_queues_.Resize(256);
    id_ = id;
    sleep_us_ = 0;
    EnableContinuousPolling();
    retries_ = 1;
    pthread_id_ = thread_.native_handle();
    pid_ = 0;
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

  /** Set the sleep cycle */
  void SetPollingFrequency(size_t sleep_us, u32 num_retries) {
    sleep_us_ = sleep_us;
    retries_ = num_retries;
    flags_.UnsetBits(WORKER_CONTINUOUS_POLLING);
  }

  /** Enable continuous polling */
  void EnableContinuousPolling() {
    flags_.SetBits(WORKER_CONTINUOUS_POLLING);
  }

  /** Disable continuous polling */
  void DisableContinuousPolling() {
    flags_.UnsetBits(WORKER_CONTINUOUS_POLLING);
  }

  /** Set the CPU affinity of this worker */
  void SetCpuAffinity(int cpu_id) {
    ProcessAffiner::SetCpuAffinity(pid_, cpu_id);
  }

  /** The work loop */
  void Loop();

 private:
  /** Worker yields for a period of time */
  void Yield() {
    if (flags_.Any(WORKER_CONTINUOUS_POLLING)) {
      return;
    }
    if (sleep_us_ > 0) {
      HERMES_THREAD_MODEL->SleepForUs(sleep_us_);
    } else {
      HERMES_THREAD_MODEL->Yield();
    }
  }

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

  /**
   * Poll work queue
   *
   * @return true if work was found, false otherwise
   * */
  void Run();

  void PollOrdered(u32 lane_id, MultiQueue *queue);

  void PollUnordered(u32 lane_id, MultiQueue *queue);
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
