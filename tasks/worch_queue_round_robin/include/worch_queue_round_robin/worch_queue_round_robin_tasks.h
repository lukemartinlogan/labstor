//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_WORCH_QUEUE_ROUND_ROBIN_TASKS_H_
#define LABSTOR_WORCH_QUEUE_ROUND_ROBIN_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/work_orchestrator/scheduler.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::worch_queue_round_robin {

/** The set of methods in the worch task */
typedef SchedulerMethod Method;

/**
 * A task to create worch_queue_round_robin
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
                            "worch_queue_round_robin", id, max_lanes,
                            num_lanes, depth, flags) {
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {}
};

/** A task to destroy worch_queue_round_robin */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               TaskStateId &state_id,
               const DomainId &domain_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

}  // namespace labstor::worch_queue_round_robin

#endif  // LABSTOR_WORCH_QUEUE_ROUND_ROBIN_TASKS_H_
