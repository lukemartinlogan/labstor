//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_worch_proc_round_robin_H_
#define LABSTOR_worch_proc_round_robin_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor/work_orchestrator/scheduler.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::worch_proc_round_robin {

/** The set of methods in the worch task */
typedef SchedulerMethod Method;

/**
 * A task to create worch_proc_round_robin
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &state_id)
      : CreateTaskStateTask(alloc, domain_id,
                            state_name,
                            "worch_proc_round_robin",
                            state_id) {
    // Custom params
  }
};

/** A task to destroy worch_proc_round_robin */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskStateId &state_id,
               const DomainId &domain_id)
      : DestroyTaskStateTask(alloc, domain_id, state_id) {}
};

/** Create admin requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a worch_proc_round_robin */
  HSHM_ALWAYS_INLINE
  void Create(const DomainId &domain_id, const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        domain_id,
        state_name,
        id_);
  }

  /** Destroy state */
  HSHM_ALWAYS_INLINE
  void Destroy(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
  }
};

}  // namespace labstor

#endif  // LABSTOR_worch_proc_round_robin_H_
