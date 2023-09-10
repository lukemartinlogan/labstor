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
  Client() {
    id_ = TaskStateId(LABSTOR_QM_CLIENT->process_queue_);
    queue_id_ = LABSTOR_QM_CLIENT->process_queue_;
  }

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
  LABSTOR_TASK_NODE_ROOT(AsyncCreate);
  template<typename ...Args>
  HSHM_ALWAYS_INLINE
  void CreateRoot(Args&& ...args) {
    auto *task = AsyncCreateRoot(std::forward<Args>(args)...);
    task->Wait();
    id_ = task->id_;
    queue_id_ = QueueId(id_);
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void DestroyRoot(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskStateRoot(domain_id, id_);
  }

  /** Call a custom method */
  template<typename TaskT>
  HSHM_ALWAYS_INLINE
  TypedPushTask<TaskT>* AsyncPush(const TaskNode &task_node,
                             const DomainId &domain_id,
                             const hipc::Pointer &subtask) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<TypedPushTask<TaskT>>(
        p, task_node, domain_id, id_, subtask);
    queue->Emplace(task->lane_hash_, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncPush);
};

}  // namespace labstor

#define LABSTOR_PROCESS_QUEUE \
  hshm::EasySingleton<labstor::proc_queue::Client>::GetInstance()

#endif  // LABSTOR_proc_queue_H_
