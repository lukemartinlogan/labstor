//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_TASKS_H_
#define LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

#include <thallium.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/list.hpp>

namespace tl = thallium;

namespace labstor::remote_queue {

#include "remote_queue_methods.h"

/**
 * A task to create remote_queue
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &id,
                u32 max_lanes, u32 num_lanes,
                u32 depth, bitfield32_t flags)
      : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                            "remote_queue", id, max_lanes,
                            num_lanes, depth, flags) {
    // Custom params
  }
};

/** A task to destroy remote_queue */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               TaskStateId &state_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**
 * Phases of the push task
 * */
class PushPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/**
 * A task to push a serialized task onto the remote queue
 * */
struct PushTask : public Task {
  IN TaskState *exec_;
  IN u32 exec_method_;
  IN std::vector<DataTransfer> *xfer_;
  IN DomainId to_domain_;
  TEMP hipc::ShmArchive<thallium::async_response> tl_future_;
  TEMP int phase_;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  PushTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id,
           TaskState *exec,
           u32 exec_method,
           std::vector<DataTransfer> &xfer,
           const DomainId &to_domain) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kPush;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    exec_ = exec;
    exec_method_ = exec_method;
    xfer_ = &xfer;
    to_domain_ = to_domain;
    phase_ = PushPhase::kStart;
  }
};

/**
 * A task to push a serialized task onto the remote queue
 * */
struct AcceptTask : public Task {
  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  AcceptTask(hipc::Allocator *alloc,
             const TaskNode &task_node,
             const DomainId &domain_id,
             const TaskStateId &state_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kAccept;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }
};

/**
 * A custom task in remote_queue
 * */
struct DisperseTask : public Task {
  IN Task *orig_task_;
  IN std::vector<DataTransfer> xfer_;
  TEMP std::vector<Task*> subtasks_;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DisperseTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               const TaskStateId &state_id,
               Task *orig_task,
               std::vector<DataTransfer> &xfer,
               size_t num_subtasks) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kDisperse;
    task_flags_.SetBits(TASK_FIRE_AND_FORGET);
    domain_id_ = domain_id;

    // Custom params
    orig_task_ = orig_task;
    xfer_ = std::move(xfer);
    subtasks_.reserve(num_subtasks);
  }
};

} // namespace labstor::remote_queue

#endif //LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_TASKS_H_
