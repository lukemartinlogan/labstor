//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_worch_proc_round_robin_H_
#define LABSTOR_worch_proc_round_robin_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor/work_orchestrator/scheduler.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::worch_proc_round_robin {

/** The set of methods in the worch task */
typedef SchedulerMethod Method;

/**
 * A task to create worch_proc_round_robin
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

/** A task to destroy worch_proc_round_robin */
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

  /** Create a worch_proc_round_robin */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &exec_name, u32 node_id) {
    id_ = TaskExecId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskExecutor(node_id,
                                      exec_name,
                                      "worch_proc_round_robin",
                                      id_);
  }

  /** Destroy executor */
  HSHM_ALWAYS_INLINE
  void Destroy(const std::string &exec_name, u32 node_id) {
    LABSTOR_ADMIN->DestroyTaskExecutor(node_id, id_);
  }
};

}  // namespace labstor

#endif  // LABSTOR_worch_proc_round_robin_H_
