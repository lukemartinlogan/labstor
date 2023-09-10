//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_bdev_H_
#define LABSTOR_bdev_H_

#include "bdev_tasks.h"

namespace hermes::bdev {

/**
 * BDEV Client API
 * */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;
  DomainId domain_id_;
  MonitorTask *monitor_task_;
  size_t max_cap_;      /**< maximum capacity of the target */
  double bandwidth_;    /**< the bandwidth of the device */
  double latency_;      /**< the latency of the device */
  float score_;         /**< Relative importance of this tier */

 public:
  /** Copy dev info */
  void CopyDevInfo(DeviceInfo &dev_info) {
    max_cap_ = dev_info.capacity_;
    bandwidth_ = dev_info.bandwidth_;
    latency_ = dev_info.latency_;
    score_ = 1;
  }

  /** Async create task state */
  HSHM_ALWAYS_INLINE
  ConstructTask* AsyncCreate(const TaskNode &task_node,
                            const DomainId &domain_id,
                            const std::string &state_name,
                            const std::string &lib_name,
                            DeviceInfo &dev_info) {
    domain_id_ = domain_id;
    id_ = TaskStateId::GetNull();
    CopyDevInfo(dev_info);
    return LABSTOR_ADMIN->AsyncCreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, lib_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(QUEUE_UNORDERED),
        dev_info);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncCreateTaskState);

  /** Complete async create task */
  void AsyncCreateComplete(ConstructTask *task) {
    if (task->IsComplete()) {
      id_ = task->id_;
      queue_id_ = QueueId(id_);
      LABSTOR_CLIENT->DelTask(task);
    }
  }

  template<typename ...Args>
  HSHM_ALWAYS_INLINE
  void CreateRoot(Args&& ...args) {
    auto *task = AsyncCreateRoot(std::forward<Args>(args)...);
    task->Wait();
    TaskNode task_node = task->task_node_;
    AsyncCreateComplete(task);
    AsyncMonitor(task_node, 100);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void DestroyRoot(const std::string &state_name) {
    LABSTOR_ADMIN->DestroyTaskStateRoot(domain_id_, id_);
    monitor_task_->SetModuleComplete();
  }

  /** BDEV monitoring task */
  HSHM_ALWAYS_INLINE
  void AsyncMonitor(const TaskNode &task_node,
                    size_t freq_ms) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    monitor_task_ = LABSTOR_CLIENT->NewTask<MonitorTask>(
        p, task_node, domain_id_, id_, freq_ms, max_cap_);
    queue->Emplace(0, p);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncMonitor);

  /** Update bdev capacity */
  void AsyncUpdateCapacity(const TaskNode &task_node,
                           ssize_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    LABSTOR_CLIENT->NewTask<UpdateCapacityTask>(
        p,
        task_node, domain_id_, id_, size);
    queue->Emplace(0, p);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncUpdateCapacity);

  /** Get bdev remaining capacity */
  HSHM_ALWAYS_INLINE
  size_t GetRemCap() const {
    return monitor_task_->rem_cap_;
  }

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  AllocTask* AsyncAllocate(const TaskNode &task_node,
                           size_t size,
                           std::vector<BufferInfo> &buffers) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<AllocTask>(
        p,
        task_node, domain_id_, id_, size, &buffers);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncAllocate);

  /** Free data */
  HSHM_ALWAYS_INLINE
  FreeTask* AsyncFree(const TaskNode &task_node,
                      const std::vector<BufferInfo> &buffers,
                      bool fire_and_forget) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<FreeTask>(
        p,
        task_node, domain_id_, id_, buffers, fire_and_forget);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncFree);

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  WriteTask* AsyncWrite(const TaskNode &task_node,
                        const char *data, size_t off, size_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<WriteTask>(
        p,
        task_node, domain_id_, id_, data, off, size);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncWrite);

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  ReadTask* AsyncRead(const TaskNode &task_node,
                      char *data, size_t off, size_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<ReadTask>(
        p,
        task_node, domain_id_, id_, data, off, size);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncRead);
};

class Server {
 public:
  ssize_t rem_cap_;

 public:
  void UpdateCapacity(MultiQueue *queue, UpdateCapacityTask *task) {
    rem_cap_ += task->diff_;
  }

  void Monitor(MultiQueue *queue, MonitorTask *task) {
    task->rem_cap_ = rem_cap_;
  }
};

}  // namespace hermes::bdev

namespace hermes {
typedef bdev::Client TargetInfo;
}  // namespace hermes

#endif  // LABSTOR_bdev_H_
