//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_worch_queue_round_robin_H_
#define LABSTOR_worch_queue_round_robin_H_

#include "worch_queue_round_robin_tasks.h"

namespace labstor::worch_queue_round_robin {

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

  /** Create a worch_queue_round_robin */
  HSHM_ALWAYS_INLINE
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    id_ = LABSTOR_CLIENT->MakeTaskStateId();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        1, 1, 4, bitfield32_t(0));
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);
};

}  // namespace labstor

#endif  // LABSTOR_worch_queue_round_robin_H_
