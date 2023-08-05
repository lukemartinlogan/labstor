//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_bdev_H_
#define LABSTOR_bdev_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "hermes/config_server.h"

namespace hermes::bdev {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kWrite = TaskMethod::kLast;
  TASK_METHOD_T kRead = TaskMethod::kLast + 1;
  TASK_METHOD_T kAlloc = TaskMethod::kLast + 2;
  TASK_METHOD_T kFree = TaskMethod::kLast + 3;
  TASK_METHOD_T kMonitor = TaskMethod::kLast + 4;
  TASK_METHOD_T kUpdateCapacity = TaskMethod::kLast + 5;
  TASK_METHOD_T kBdevLast = TaskMethod::kLast + 6;
};

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  IN DeviceInfo *info_;

  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const std::string &lib_name,
                const TaskStateId &id,
                u32 max_lanes, u32 num_lanes,
                u32 depth, bitfield32_t flags,
                DeviceInfo &info)
  : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                        lib_name, id, max_lanes,
                        num_lanes, depth, flags) {
    // Custom params
    info_ = &info;
  }
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               TaskStateId &state_id,
               const DomainId &domain_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**
 * A custom task in bdev
 * */
struct AllocTask : public LocalTask {
  IN size_t size_;  /**< Size in buf */
  OUT std::vector<BufferInfo> *buffers_;
  OUT size_t alloc_size_;

  HSHM_ALWAYS_INLINE
  AllocTask(hipc::Allocator *alloc,
            const TaskNode &task_node,
            const DomainId &domain_id,
            const TaskStateId &state_id,
            size_t size,
            std::vector<BufferInfo> *buffers) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kAlloc;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    size_ = size;
    buffers_ = buffers;
  }
};

/**
 * A custom task in bdev
 * */
struct FreeTask : public LocalTask {
  IN const std::vector<BufferInfo> *buffers_;

  HSHM_ALWAYS_INLINE
  FreeTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id,
           const std::vector<BufferInfo> &buffers) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kFree;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buffers_ = &buffers;
  }
};

/**
 * A custom task in bdev
 * */
struct WriteTask : public LocalTask {
  IN const char *buf_;    /**< Data in memory */
  IN size_t disk_off_;   /**< Offset on disk */
  IN size_t size_;  /**< Size in buf */

  HSHM_ALWAYS_INLINE
  WriteTask(hipc::Allocator *alloc,
            const TaskNode &task_node,
            const DomainId &domain_id,
            const TaskStateId &state_id,
            const char *buf,
            size_t disk_off,
            size_t size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kWrite;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buf_ = buf;
    disk_off_ = disk_off;
    size_ = size;
  }
};

/**
 * A custom task in bdev
 * */
struct ReadTask : public LocalTask {
  IN char *buf_;    /**< Data in memory */
  IN size_t disk_off_;   /**< Offset on disk */
  IN size_t size_;  /**< Size in disk buf */

  HSHM_ALWAYS_INLINE
  ReadTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id,
           char *buf,
           size_t disk_off,
           size_t size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kRead;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buf_ = buf;
    disk_off_ = disk_off;
    size_ = size;
  }
};

/** A task to monitor bdev statistics */
struct MonitorTask : public LocalTask {
  IN size_t freq_ms_;  /**< Frequency in ms */
  OUT size_t rem_cap_; /**< Remaining capacity of the target */

  HSHM_ALWAYS_INLINE
  MonitorTask(hipc::Allocator *alloc,
              const TaskNode &task_node,
              const DomainId &domain_id,
              const TaskStateId &state_id,
              size_t freq_ms,
              size_t rem_cap) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kMonitor;
    task_flags_.SetBits(TASK_LONG_RUNNING);
    domain_id_ = domain_id;

    // Custom
    freq_ms_ = freq_ms;
    rem_cap_ = rem_cap;
  }
};

/** A task to update bdev capacity */
struct UpdateCapacityTask : public LocalTask {
  IN ssize_t diff_;

  HSHM_ALWAYS_INLINE
  UpdateCapacityTask(hipc::Allocator *alloc,
                     const TaskNode &task_node,
                     const DomainId &domain_id,
                     const TaskStateId &state_id,
                     ssize_t diff) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kUpdateCapacity;
    task_flags_.SetBits(TASK_FIRE_AND_FORGET);
    domain_id_ = domain_id;

    // Custom
    diff_ = diff;
  }
};

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
        bitfield32_t(0),
        dev_info);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncCreateTaskState);

  /** Create a bdev */
  template<typename ...Args>
  HSHM_ALWAYS_INLINE
  void Create(TaskNode task_node, Args&& ...args) {
    auto *task = AsyncCreate(std::forward<Args>(args)...);
    task->Wait();
    id_ = task->id_;
    queue_id_ = QueueId(id_);
    Monitor(task_node, 100);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const std::string &state_name) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id_, id_);
    monitor_task_->SetComplete();
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /** BDEV monitoring task */
  HSHM_ALWAYS_INLINE
  void Monitor(const TaskNode &task_node,
               size_t freq_ms) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    monitor_task_ = LABSTOR_CLIENT->NewTask<MonitorTask>(
        p, task_node, domain_id_, id_, freq_ms, max_cap_);
    queue->Emplace(0, p);
  }
  LABSTOR_TASK_NODE_ROOT(Monitor);

  /** Update bdev capacity */
  void UpdateCapacity(const TaskNode &task_node,
                      ssize_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    LABSTOR_CLIENT->NewTask<UpdateCapacityTask>(
        p,
        task_node, domain_id_, id_, size);
    queue->Emplace(0, p);
  }
  LABSTOR_TASK_NODE_ROOT(UpdateCapacity);

  /** Get bdev remaining capacity */
  HSHM_ALWAYS_INLINE
  size_t GetRemCap() const {
    return monitor_task_->rem_cap_;
  }
  LABSTOR_TASK_NODE_ROOT(GetRemCap);

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  AllocTask* AsyncAllocate(const TaskNode &task_node,
                           size_t size,
                           std::vector<BufferInfo> &buffers) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
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
                      const std::vector<BufferInfo> &buffers) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<FreeTask>(
        p,
        task_node, domain_id_, id_, buffers);
    queue->Emplace(0, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncFree);

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  WriteTask* AsyncWrite(const TaskNode &task_node,
                        const char *data, size_t off, size_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
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
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
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
