//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_adapters_H_
#define LABSTOR_hermes_adapters_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::hermes_adapters {

/** The set of methods in the hermes_adapters task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = TaskMethod::kLast;
};

/**
 * A task to create hermes_adapters
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
                        "hermes_adapters", id, max_lanes,
                        num_lanes, depth, flags) {
    // Custom params
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    // Custom params
  }
};

/** A task to destroy hermes_adapters */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               const TaskStateId &state_id)
  : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**
 * A custom task in hermes_adapters
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
};

/** Create hermes_adapters requests */
class Client : public TaskLibClient {

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a hermes_adapters */
  HSHM_ALWAYS_INLINE
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(0));
    queue_id_ = QueueId(id_);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }

  /** Call a custom method */
  HSHM_ALWAYS_INLINE
  void Custom(const TaskNode &task_node,
              const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<CustomTask>(
        p, task_node, domain_id, id_);
    queue->Emplace(0, p);
    task->Wait();
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_adapters_H_
