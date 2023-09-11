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
#include "labstor/network/rpc_thallium.h"

namespace labstor {

#define WORKER_CONTINUOUS_POLLING BIT_OPT(u32, 0)

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
  // ABT_thread tl_thread_;
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
  std::unordered_map<hshm::charbuf, TaskNode> group_map_;  /** Determine if a task can be executed right now */
  hshm::charbuf group_;  /** The current group */


 public:
  /** Constructor */
  Worker(u32 id, ABT_xstream &xstream) {
    poll_queues_.Resize(1024);
    relinquish_queues_.Resize(1024);
    id_ = id;
    sleep_us_ = 0;
    EnableContinuousPolling();
    retries_ = 1;
    pid_ = 0;
    thread_ = std::make_unique<std::thread>(&Worker::Loop, this);
    pthread_id_ = thread_->native_handle();
    // TODO(llogan): implement reserve for group
    group_.resize(512);
    group_.resize(0);
    /* int ret = ABT_thread_create_on_xstream(xstream,
                                           [](void *args) { ((Worker*)args)->Loop(); }, this,
                                           ABT_THREAD_ATTR_NULL, &tl_thread_);
    if (ret != ABT_SUCCESS) {
      HELOG(kFatal, "Couldn't spawn worker");
    }*/
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
        // HILOG(kDebug, "Scheduled queue {} (lane {})", entry.queue_->id_, entry.lane_);
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

  HSHM_ALWAYS_INLINE
  bool CheckTaskGroup(Task *task, TaskState *exec, TaskNode node, const bool &is_remote) {
    if (is_remote || task->IsStarted()) {
      return true;
    }
    int ret = exec->GetGroup(task->method_, task, group_);
    if (ret == TASK_UNORDERED || task->IsUnordered()) {
      HILOG(kDebug, "(node {}) Task {} is unordered, so count remains 0 worker={}",
            LABSTOR_CLIENT->node_id_, task->task_node_, id_);
      return true;
    }

#ifdef DEBUG
    // TODO(llogan): remove
    std::stringstream ss;
    for (int i = 0; i < group_.size(); ++i) {
      ss << std::to_string((int)group_[i]);
    }
#endif

    auto it = group_map_.find(group_);
    if (it == group_map_.end()) {
      node.node_depth_ = 1;
      group_map_.emplace(group_, node);
      HILOG(kDebug, "(node {}) Increasing depth of group to {} worker={}",
            LABSTOR_CLIENT->node_id_, node.node_depth_, id_);
      return true;
    }
    TaskNode &node_cmp = it->second;
    if (node_cmp.root_ == node.root_) {
      node_cmp.node_depth_ += 1;
      HILOG(kDebug, "(node {}) Increasing depth of group to {} worker={}",
            LABSTOR_CLIENT->node_id_, node_cmp.node_depth_, id_);
      return true;
    }
    return false;
  }

  HSHM_ALWAYS_INLINE
  void RemoveTaskGroup(Task *task, TaskState *exec, const bool &is_remote) {
    if (is_remote) {
      return;
    }
    int ret = exec->GetGroup(task->method_, task, group_);
    if (ret == TASK_UNORDERED || task->IsUnordered() || task->method_ < 2) {
      HILOG(kDebug, "(node {}) Decreasing depth of group remains 0 (task_node={} worker={})",
            LABSTOR_CLIENT->node_id_, task->task_node_, id_);
      return;
    }

#ifdef DEBUG
    // TODO(llogan): remove
    std::stringstream ss;
    for (int i = 0; i < group_.size(); ++i) {
      ss << std::to_string((int)group_[i]);
    }
#endif

    TaskNode &node_cmp = group_map_[group_];
    if (node_cmp.node_depth_ == 0) {
      HELOG(kFatal, "(node {}) Group {} depth is already 0 (task_node={} worker={})",
            LABSTOR_CLIENT->node_id_, task->task_node_, id_);
    }
    node_cmp.node_depth_ -= 1;
    HILOG(kDebug, "(node {}) Decreasing depth of to {} (task_node={} worker={})",
          LABSTOR_CLIENT->node_id_, node_cmp.node_depth_, task->task_node_, id_);
    if (node_cmp.node_depth_ == 0) {
      group_map_.erase(group_);
    }
  }

  HSHM_ALWAYS_INLINE
  void EndTask(MultiQueue *queue, u32 lane_id, Task *task, int &off) {
    PopTask(queue, lane_id, off);
    if (task->IsFireAndForget()) {
      LABSTOR_CLIENT->DelTask(task);
    } else {
      task->SetComplete();
    }
  }

  HSHM_ALWAYS_INLINE
  void PopTask(MultiQueue *queue, u32 lane_id, int &off) {
    LaneData data;
    if (off == 0) {
      queue->Pop(lane_id, data);
    } else {
      off += 1;
    }
  }

  void PollGrouped(u32 lane_id, MultiQueue *queue);
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_WORKER_H_
