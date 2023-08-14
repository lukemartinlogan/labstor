//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_H_
#define LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "hermes/config_server.h"
#include "labstor/labstor_namespace.h"

namespace hermes::bdev {

#include "bdev_methods.h"

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  IN DeviceInfo info_;


  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
    info_ = info;
  }
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
  IN const std::vector<BufferInfo>* buffers_;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
  IN size_t disk_off_;    /**< Offset on disk */
  IN size_t size_;        /**< Size in buf */

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
  IN char *buf_;         /**< Data in memory */
  IN size_t disk_off_;   /**< Offset on disk */
  IN size_t size_;       /**< Size in disk buf */

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

}  // namespace hermes::bdev

#endif  // LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_H_
