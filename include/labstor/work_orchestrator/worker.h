//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_

#include "labstor/labstor_types.h"
#include "labstor/queue_manager/queue_manager_runtime.h"
#include <thread>
#include <queue>
#include "affinity.h"

namespace labstor {

#define WORKER_CONTINUOUS_POLLING (1 << 0)

/** Uniquely identify a queue lane */
struct WorkEntry {
  u32 lane_;
  MultiQueue *queue_;

  /** Default constructor */
  HSHM_ALWAYS_INLINE
  WorkEntry() = default;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  WorkEntry(u32 lane, MultiQueue *queue) : lane_(lane), queue_(queue) {}

  /** Copy constructor */
  HSHM_ALWAYS_INLINE
  WorkEntry(const WorkEntry &other) {
    queue_ = other.queue_;
    lane_ = other.lane_;
  }

  /** Copy assignment */
  HSHM_ALWAYS_INLINE
      WorkEntry
  &
  operator=(const WorkEntry &other) {
    if (this != &other) {
      queue_ = other.queue_;
      lane_ = other.lane_;
    }
    return *this;
  }

  /** Move constructor */
  HSHM_ALWAYS_INLINE
  WorkEntry(WorkEntry &&other) noexcept {
    queue_ = other.queue_;
    lane_ = other.lane_;
  }

  /** Move assignment */
  HSHM_ALWAYS_INLINE
      WorkEntry
  &
  operator=(WorkEntry &&other) noexcept {
    if (this != &other) {
      queue_ = other.queue_;
      lane_ = other.lane_;
    }
    return *this;
  }

  /** Check if null */
  [[nodiscard]]
  HSHM_ALWAYS_INLINE bool IsNull() const {
    return queue_->IsNull();
  }

  /** Equality operator */
  HSHM_ALWAYS_INLINE
  bool operator==(const WorkEntry &other) const {
    return queue_ == other.queue_ && lane_ == other.lane_;
  }
};

}  // namespace labstor

namespace std {
/** Hash function for WorkEntry */
template<>
struct hash<labstor::WorkEntry> {
  HSHM_ALWAYS_INLINE
      std::size_t
  operator()(const labstor::WorkEntry &key) const {
    return std::hash<labstor::MultiQueue*>{}(key.queue_) +
        std::hash<u32>{}(key.lane_);
  }
};
}  // namespace std

namespace labstor {

class Worker {
 public:
  u32 id_;  /**< Unique identifier of this worker */
  std::unique_ptr<std::thread> thread_;  /**< The worker thread handle */
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
  std::unordered_map<WorkEntry, std::queue<Task*>> active_tasks_;  /**< The set of active tasks */

 public:
  /** Constructor */
  Worker(u32 id) {
    poll_queues_.Resize(1024);
    relinquish_queues_.Resize(1024);
    id_ = id;
    sleep_us_ = 0;
    EnableContinuousPolling();
    retries_ = 1;
    pid_ = 0;
    thread_ = std::make_unique<std::thread>(&Worker::Loop, this);
    pthread_id_ = thread_->native_handle();
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

  void PollUnordered(u32 lane_id, MultiQueue *queue);
  void PollOrdered(u32 lane_id, MultiQueue *queue);
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
