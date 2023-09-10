//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_TASK_NAME_H_
#define LABSTOR_TASK_NAME_H_

#include "TASK_NAME_tasks.h"

namespace labstor::TASK_NAME {

/** Create TASK_NAME requests */
class Client : public TaskLibClient {

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

  /** Create a TASK_NAME */
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
  HSHM_ALWAYS_INLINE
  void AsyncCustomConstruct(CustomTask *task,
                            const TaskNode &task_node,
                            const DomainId &domain_id) {
    LABSTOR_CLIENT->ConstructTask<CustomTask>(
        task, task_node, domain_id, id_);
  }
  HSHM_ALWAYS_INLINE
  void CustomRoot(const DomainId &domain_id) {
    LPointer<TypedPushTask<CustomTask>> task = AsyncCustomRoot(domain_id);
    task.ptr_->Wait();
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(Custom);
};

}  // namespace labstor

#endif  // LABSTOR_TASK_NAME_H_
