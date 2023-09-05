//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_remote_queue_H_
#define LABSTOR_remote_queue_H_

#include "remote_queue_tasks.h"

namespace labstor::remote_queue {

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
                             const std::string &state_name,
                             const TaskStateId &state_id) {
    id_ = state_id;
    return LABSTOR_ADMIN->AsyncCreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(QUEUE_UNORDERED));
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
                std::vector<DomainId> &domain_ids) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);

    // Serialize task + create the wait task
    HILOG(kInfo, "Beginning dispersion for (task_node={}, task_state={}, method={})",
          orig_task->task_node_, orig_task->task_state_, orig_task->method_)
    BinaryOutputArchive<true> ar(DomainId::GetNode(LABSTOR_QM_CLIENT->node_id_));
    auto xfer = exec->SaveStart(orig_task->method_, ar, orig_task);

    // Create subtasks
    exec->ReplicateStart(orig_task->method_, domain_ids.size(), orig_task);
    LABSTOR_CLIENT->NewTask<PushTask>(
        p, orig_task->task_node_, DomainId::GetLocal(), id_,
        domain_ids, orig_task, exec, orig_task->method_, xfer);
    queue->Emplace(orig_task->lane_hash_, p);
  }
  LABSTOR_TASK_NODE_ROOT(Custom);

  /** Spawn task to accept new connections */
//  HSHM_ALWAYS_INLINE
//  AcceptTask* AsyncAcceptThread() {
//    hipc::Pointer p;
//    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
//    auto *task = LABSTOR_CLIENT->NewTask<AcceptTask>(
//        p, TaskNode::GetNull(), DomainId::GetLocal(), id_);
//    queue->Emplace(0, p);
//    return task;
//  }
};

}  // namespace labstor

#endif  // LABSTOR_remote_queue_H_
