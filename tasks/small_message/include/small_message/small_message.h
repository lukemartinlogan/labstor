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
struct ConstructTask : public Task {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskExecId &exec_id,
                u32 node_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = exec_id;
    method_ = Method::kConstruct;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Custom params
  }
};

/** A task to destroy small_message */
struct DestructTask : public Task {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskExecId &exec_id,
               u32 node_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = exec_id;
    method_ = Method::kDestruct;
    task_flags_.SetBits(0);
    node_id_ = node_id;
  }
};

/**
 * A custom task in small_message
 * */
struct CustomTask : public Task {
  OUT int ret_;

  HSHM_ALWAYS_INLINE
  CustomTask(hipc::Allocator *alloc,
             const TaskExecId &exec_id,
             u32 node_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_exec_ = exec_id;
    method_ = Method::kCustom;
    task_flags_.SetBits(0);
    node_id_ = node_id;

    // Custom params
  }
};

/** Create admin requests */
class Client {
 public:
  TaskExecId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a small_message */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &exec_name, u32 node_id) {
    id_ = TaskExecId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskExecutor(node_id,
                                      exec_name,
                                      "small_message",
                                      id_);
    queue_id_ = QueueId(id_.unique_, id_.node_id_);
    LABSTOR_ADMIN->CreateQueue(node_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
  }

  /** Destroy executor + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const std::string &exec_name, u32 node_id) {
    LABSTOR_ADMIN->DestroyTaskExecutor(node_id, id_);
    LABSTOR_ADMIN->DestroyQueue(node_id, queue_id_);
  }
};

}  // namespace labstor

#endif  // LABSTOR_small_message_H_
