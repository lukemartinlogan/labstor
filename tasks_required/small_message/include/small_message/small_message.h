//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_small_message_H_
#define LABSTOR_small_message_H_

#include "small_message_tasks.h"

namespace labstor::small_message {

/** Create admin requests */
class Client : public TaskLibClient {

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a small_message */
  HSHM_ALWAYS_INLINE
  void CreateRoot(const DomainId &domain_id,
                  const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskStateRoot<ConstructTask>(
        domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(QUEUE_LOW_LATENCY | QUEUE_UNORDERED));
    queue_id_ = QueueId(id_);
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    HILOG(kDebug, "Created small_message queue {}", queue->num_lanes_);
  }

  /** Destroy state + queue */
  HSHM_ALWAYS_INLINE
  void DestroyRoot(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskStateRoot(domain_id, id_);
  }

  /** Metadata task */
  MdTask* AsyncMd(const TaskNode &task_node,
                  const DomainId &domain_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    auto *task = LABSTOR_CLIENT->NewTask<MdTask>(
        p,
        task_node, domain_id, id_);
    queue->Emplace(3, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncMd);
  int MdRoot(const DomainId &domain_id) {
    auto *task = AsyncMdRoot(domain_id);
    task->Wait();
    int ret = task->ret_[0];
    LABSTOR_CLIENT->DelTask(task);
    return ret;
  }

  /** Io task */
  void AsyncIoConstruct(IoTask *task, const TaskNode &task_node,
                        const DomainId &domain_id) {
    LABSTOR_CLIENT->ConstructTask<IoTask>(
        task, task_node, domain_id, id_);
  }
  int IoRoot(const DomainId &domain_id) {
    LPointer<TypedPushTask<IoTask>> push_task = AsyncIoRoot(domain_id);
    push_task.ptr_->Wait();
    IoTask *task = push_task.ptr_->subtask_ptr_;
    int ret = task->ret_;
    LABSTOR_CLIENT->DelTask(task);
    return ret;
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(Io)
};

}  // namespace labstor

#endif  // LABSTOR_small_message_H_
