//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_worch_queue_round_robin_H_
#define LABSTOR_worch_queue_round_robin_H_

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
struct ConstructTask : public Task {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskStateId &state_id,
                const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kConstruct;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
  }
};

/** A task to destroy worch_queue_round_robin */
struct DestructTask : public Task {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskStateId &state_id,
               const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kDestruct;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }
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

  /** Create a worch_queue_round_robin */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &state_name, const DomainId &domain_id) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState(domain_id,
                                      state_name,
                                      "worch_queue_round_robin",
                                      id_);
  }

  /** Destroy task state */
  HSHM_ALWAYS_INLINE
  void Destroy(const std::string &state_name, const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
  }
};

}  // namespace labstor

#endif  // LABSTOR_worch_queue_round_robin_H_
