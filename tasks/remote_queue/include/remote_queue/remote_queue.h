//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_remote_queue_H_
#define LABSTOR_remote_queue_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

#include <thallium.hpp>
#include <thallium/serialization/stl/pair.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/list.hpp>

namespace tl = thallium;

namespace labstor::remote_queue {

/** The set of methods in the remote_queue task */
struct Method : public TaskMethod {
  TASK_METHOD_T kDisperse = TaskMethod::kLast;
  TASK_METHOD_T kPush = TaskMethod::kLast + 1;
  TASK_METHOD_T kAccept = TaskMethod::kLast + 2;
};

/**
 * A task to create remote_queue
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
                        "remote_queue", id, max_lanes,
                        num_lanes, depth, flags) {
    // Custom params
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    // Custom params
  }
};

/** A task to destroy remote_queue */
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
 * Phases of the push task
 * */
class PushPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/**
 * A task to push a serialized task onto the remote queue
 * */
struct PushTask : public Task {
  IN TaskState *exec_;
  IN u32 exec_method_;
  IN std::vector<DataTransfer> *xfer_;
  IN DomainId to_domain_;
  TEMP hipc::ShmArchive<thallium::async_response> tl_future_;
  TEMP int phase_;

  HSHM_ALWAYS_INLINE
  PushTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id,
           TaskState *exec,
           u32 exec_method,
           std::vector<DataTransfer> &xfer,
           const DomainId &to_domain) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kPush;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    exec_ = exec;
    exec_method_ = exec_method;
    xfer_ = &xfer;
    to_domain_ = to_domain;
    phase_ = PushPhase::kStart;
  }
};

/**
 * A task to push a serialized task onto the remote queue
 * */
struct AcceptTask : public Task {
  HSHM_ALWAYS_INLINE
  AcceptTask(hipc::Allocator *alloc,
           const TaskNode &task_node,
           const DomainId &domain_id,
           const TaskStateId &state_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kAccept;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }
};


/**
 * A custom task in remote_queue
 * */
struct DisperseTask : public Task {
  IN Task *orig_task_;
  IN std::vector<DataTransfer> xfer_;
  TEMP std::vector<Task*> subtasks_;

  HSHM_ALWAYS_INLINE
  DisperseTask(hipc::Allocator *alloc,
             const TaskNode &task_node,
             const DomainId &domain_id,
             const TaskStateId &state_id,
             Task *orig_task,
             std::vector<DataTransfer> &xfer,
             size_t num_subtasks) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = state_id;
    method_ = Method::kDisperse;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    orig_task_ = orig_task;
    xfer_ = std::move(xfer);
    subtasks_.reserve(num_subtasks);
  }
};

/**
 * Create remote_queue requests
 *
 * This is ONLY used in the Hermes runtime, and
 * should never be called in client programs!!!
 * */
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

  /** Create a remote_queue */
  template<typename ...Args>
  HSHM_ALWAYS_INLINE
  void Create(Args&& ...args) {
    auto *task = AsyncCreate(std::forward<Args>(args)...);
    task->Wait();
    id_ = task->id_;
    queue_id_ = QueueId(id_);
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Init */
  void Init(TaskStateId state_id) {
    id_ = state_id;
    queue_id_ = QueueId(id_);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /** Call a custom method */
  HSHM_ALWAYS_INLINE
  void Disperse(Task *orig_task,
                TaskState *exec,
                const std::vector<DomainId> &domain_ids) {
    hipc::Pointer p, disperse_p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);

    // Serialize task + create the wait task
    BinaryOutputArchive<true> ar(DomainId::GetNode(LABSTOR_QM_CLIENT->node_id_));
    auto xfer = exec->SaveStart(orig_task->method_, ar, orig_task);
    auto *wait_task = LABSTOR_CLIENT->NewTask<DisperseTask>(
        disperse_p, orig_task->task_node_, DomainId::GetLocal(), id_, orig_task, xfer, domain_ids.size());

    // Create subtasks
    for (auto &node_id : domain_ids) {
      auto *sub_task = LABSTOR_CLIENT->NewTask<PushTask>(
          p, orig_task->task_node_, DomainId::GetLocal(), id_,
          exec, orig_task->method_, wait_task->xfer_, node_id);
      wait_task->subtasks_.push_back(sub_task);
      queue->Emplace(orig_task->lane_hash_, p);
    }

    // Enqueue wait task
    queue->Emplace(orig_task->lane_hash_, disperse_p);
  }
  LABSTOR_TASK_NODE_ROOT(Custom);

  /** Spawn task to accept new connections */
  HSHM_ALWAYS_INLINE
  AcceptTask* AsyncAcceptThread() {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<AcceptTask>(
        p, TaskNode::GetNull(), DomainId::GetLocal(), id_);
    queue->Emplace(0, p);
    return task;
  }
};

}  // namespace labstor

#endif  // LABSTOR_remote_queue_H_
