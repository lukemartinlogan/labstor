//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_proc_queue_H_
#define LABSTOR_proc_queue_H_

#include "proc_queue_tasks.h"

namespace labstor::proc_queue {

/** Create proc_queue requests */
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

  /** Create a proc_queue */
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

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /** Call a custom method */
  HSHM_ALWAYS_INLINE
  void Custom(const TaskNode &task_node,
              const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_, task_node.IsRoot());
    auto *task = LABSTOR_CLIENT->NewTask<CustomTask>(
        p, task_node, domain_id, id_);
    queue->Emplace(0, p);
    task->Wait();
  }
  LABSTOR_TASK_NODE_ROOT(Custom);
};

}  // namespace labstor

#endif  // LABSTOR_proc_queue_H_
