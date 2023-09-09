//
// Created by lukemartinlogan on 8/11/23.
//

#ifndef LABSTOR_TASKS_TASK_TEMPL_INCLUDE_proc_queue_proc_queue_TASKS_H_
#define LABSTOR_TASKS_TASK_TEMPL_INCLUDE_proc_queue_proc_queue_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "proc_queue_methods.h"
#include "labstor/labstor_namespace.h"

namespace labstor::proc_queue {

#include "proc_queue_tasks.h"

/**
 * A task to create proc_queue
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(hipc::Allocator *alloc)
  : CreateTaskStateTask(alloc) {}

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
                            "proc_queue", id, max_lanes,
                            num_lanes, depth, flags) {
    // Custom params
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    // Custom params
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    return TASK_UNORDERED;
  }
};

/** A task to destroy proc_queue */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(hipc::Allocator *alloc)
  : DestroyTaskStateTask(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               TaskStateId &state_id)
  : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    return TASK_UNORDERED;
  }
};

class PushTaskPhase {
 public:
  TASK_METHOD_T kSchedule = 0;
  TASK_METHOD_T kWaitSchedule = 1;
};

/**
 * Push a task into the per-process queue
 * */
struct PushTask : public Task, TaskFlags<TF_LOCAL> {
  IN hipc::Pointer subtask_;
  TEMP Task *subtask_ptr_;
  TEMP int phase_ = PushTaskPhase::kSchedule;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  PushTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  PushTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id,
           const hipc::Pointer &subtask) : Task(alloc) {
    // Initialize task
    hshm::NodeThreadId tid;
    task_node_ = task_node;
    lane_hash_ = tid.bits_.tid_ + tid.bits_.pid_;
    task_state_ = state_id;
    method_ = Method::kPush;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    subtask_ = subtask;
    subtask_ptr_ = (Task*)LABSTOR_CLIENT->GetPrivatePointer(subtask_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    group.resize(sizeof(u32));
    memcpy(group.data(), &lane_hash_, sizeof(u32));
    return 0;
  }
};

}  // namespace labstor::proc_queue

#endif  // LABSTOR_TASKS_TASK_TEMPL_INCLUDE_proc_queue_proc_queue_TASKS_H_
