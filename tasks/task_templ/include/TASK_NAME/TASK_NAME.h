//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASK_NAME_H_
#define LABSTOR_TASK_NAME_H_

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
  ConstructTask(CREATE_TASK_STATE_ARGS)
  : CreateTaskStateTask(PASS_CREATE_TASK_STATE_ARGS("TASK_NAME")) {
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
};

/** Create TASK_NAME requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Async create a task state */
  HSHM_ALWAYS_INLINE
  ConstructTask* AsyncCreate(const TaskNode &task_node,
                   const DomainId &domain_id,
                   const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    return LABSTOR_ADMIN->AsyncCreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(0));
  }

  /** Finish Async Create */
  bool AsyncCreateIsComplete(ConstructTask *create_task) {
    if (create_task->IsComplete()) {
      id_ = create_task->id_;
      queue_id_ = QueueId(id_);
      LABSTOR_CLIENT->DelTask(create_task);
      return true;
    }
    return false;
  }

  /** Create a TASK_NAME */
  HSHM_ALWAYS_INLINE
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    auto *task = AsyncCreate(task_node, domain_id, state_name);
    task->Wait();
    AsyncCreateIsComplete(task);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(task_node, domain_id, queue_id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /** Call a custom method */
  HSHM_ALWAYS_INLINE
  void Custom(const TaskNode &task_node,
              const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<CustomTask>(
        p, task_node, domain_id, id_);
    queue->Emplace(0, p);
    task->Wait();
  }
  LABSTOR_TASK_NODE_ROOT(Custom);
};

}  // namespace labstor

#endif  // LABSTOR_TASK_NAME_H_
