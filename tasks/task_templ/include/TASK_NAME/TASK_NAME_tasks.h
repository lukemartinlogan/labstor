//
// Created by lukemartinlogan on 8/11/23.
//

#ifndef LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_TASKS_H_
#define LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::TASK_NAME {

/** The set of methods in the TASK_NAME task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = TaskMethod::kLast;
};

/**
 * A task to create TASK_NAME
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &id,
                u32 max_lanes, u32 num_lanes,
                u32 depth, bitfield32_t flags)
      : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                            "TASK_NAME", id, max_lanes,
                            num_lanes, depth, flags) {
    // Custom params
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    // Custom params
  }
};

/** A task to destroy TASK_NAME */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               TaskStateId &state_id)
  : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**
 * A custom task in TASK_NAME
 * */
struct CustomTask : public Task {
  HSHM_ALWAYS_INLINE
  CustomTask(hipc::Allocator *alloc,
             const TaskNode &task_node,
             const DomainId &domain_id,
             const TaskStateId &state_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kCustom;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};

}  // namespace labstor::TASK_NAME

#endif  // LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_TASKS_H_
