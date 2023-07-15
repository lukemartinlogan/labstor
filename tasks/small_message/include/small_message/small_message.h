//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_small_message_H_
#define LABSTOR_small_message_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::small_message {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = TaskMethod::kLast;
};

/**
 * A task to create small_message
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
                            "small_message",
                            state_id) {
    // Custom params
  }
};

/** A task to destroy small_message */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskStateId &state_id,
               const DomainId &domain_id)
      : DestroyTaskStateTask(alloc, domain_id, state_id) {}
};

/**
 * A custom task in small_message
 * */
struct CustomTask : public Task {
  OUT int ret_;

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

  /** Create a small_message */
  HSHM_ALWAYS_INLINE
  void Create(const DomainId &domain_id, const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        domain_id,
        state_name,
        id_);
    queue_id_ = QueueId(id_);
    LABSTOR_ADMIN->CreateQueue(domain_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    HILOG(kDebug, "Created small_message queue {}", queue->num_lanes_);
  }

  /** Destroy state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(domain_id, queue_id_);
  }

  /** Custom task */
  int Custom(const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<CustomTask>(
        p,
        id_, domain_id);
    queue->Emplace(3, p);
    task->Wait();
    int ret = task->ret_;
    return ret;
  }
};

}  // namespace labstor

#endif  // LABSTOR_small_message_H_
