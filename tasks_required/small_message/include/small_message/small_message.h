//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_small_message_H_
#define LABSTOR_small_message_H_

#include "small_message_tasks.h"

namespace labstor::small_message {

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
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(QUEUE_LOW_LATENCY | QUEUE_UNORDERED));
    queue_id_ = QueueId(id_);
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    HILOG(kDebug, "Created small_message queue {}", queue->num_lanes_);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /** Metadata task */
  int Md(const TaskNode &task_node,
         const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<MdTask>(
        p,
        task_node, domain_id, id_);
    queue->Emplace(3, p);
    task->Wait();
    int ret = task->ret_;
    LABSTOR_CLIENT->DelTask(task);
    return ret;
  }
  LABSTOR_TASK_NODE_ROOT(Md);

  /** Io task */
  int Io(const TaskNode &task_node,
         const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<IoTask>(
        p,
        task_node, domain_id, id_);
    queue->Emplace(3, p);
    task->Wait();
    int ret = task->ret_;
    LABSTOR_CLIENT->DelTask(task);
    return ret;
  }
  LABSTOR_TASK_NODE_ROOT(Io);
};

}  // namespace labstor

#endif  // LABSTOR_small_message_H_
