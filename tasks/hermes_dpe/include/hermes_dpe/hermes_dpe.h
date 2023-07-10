//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_dpe_H_
#define LABSTOR_hermes_dpe_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"

namespace hermes::dpe {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = TaskMethod::kLast;
};

/**
 * A task to create hermes_dpe
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

/** A task to destroy hermes_dpe */
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

/**
 * A custom task in hermes_dpe
 * */
struct CustomTask : public Task {
  HSHM_ALWAYS_INLINE
  CustomTask(hipc::Allocator *alloc,
             const TaskStateId &state_id,
             const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kCustom;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
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

  /** Create a hermes_dpe */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &state_name, const DomainId &domain_id) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState(domain_id,
                                      state_name,
                                      "hermes_dpe",
                                      id_);
    queue_id_ = QueueId(id_);
    LABSTOR_ADMIN->CreateQueue(domain_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const std::string &state_name, const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(domain_id, queue_id_);
  }

  /** Create a queue with an ID */
  HSHM_ALWAYS_INLINE
  void Custom(const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = queue->Allocate<CustomTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, domain_id);
    queue->Emplace(0, p);
    task->Wait();
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_dpe_H_
