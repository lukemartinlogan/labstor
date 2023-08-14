//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_TASKS_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_TASKS_H_

#include "labstor/work_orchestrator/scheduler.h"
#include "labstor/api/labstor_client.h"
#include "labstor/queue_manager/queue_manager_client.h"

namespace labstor::Admin {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kRegisterTaskLib = TaskMethod::kLast + 0;
  TASK_METHOD_T kDestroyTaskLib = TaskMethod::kLast + 1;
  TASK_METHOD_T kCreateTaskState = TaskMethod::kLast + 2;
  TASK_METHOD_T kGetTaskStateId = TaskMethod::kLast + 3;
  TASK_METHOD_T kDestroyTaskState = TaskMethod::kLast + 4;
  TASK_METHOD_T kStopRuntime = TaskMethod::kLast + 5;
  TASK_METHOD_T kSetWorkOrchestratorQueuePolicy = TaskMethod::kLast + 6;
  TASK_METHOD_T kSetWorkOrchestratorProcessPolicy = TaskMethod::kLast + 7;
};

/** A template to register or destroy a task library */
template<int method>
struct RegisterTaskLibTaskTempl : public Task, SrlFlags<true, true> {
  IN hipc::ShmArchive<hipc::string>
  lib_name_;
  OUT TaskStateId
  id_;

  /** SHM default constructor */
  RegisterTaskLibTaskTempl(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  RegisterTaskLibTaskTempl(hipc::Allocator *alloc,
                           const TaskNode &task_node,
                           const DomainId &domain_id,
                           const std::string &lib_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    if constexpr(method == 0)
    {
      method_ = Method::kRegisterTaskLib;
    } else {
      method_ = Method::kDestroyTaskLib;
    }
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize QueueManager task
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
  }

  /** Destructor */
  ~RegisterTaskLibTaskTempl() {
    HSHM_DESTROY_AR(lib_name_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(lib_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(id_);
  }

  TASK_SERIALIZE
};

/** A task to register a Task Library */
using RegisterTaskLibTask = RegisterTaskLibTaskTempl<0>;

/** A task to destroy a Task Library */
using DestroyTaskLibTask = RegisterTaskLibTaskTempl<1>;

class CreateTaskStatePhase {
 public:
  // NOTE(llogan): kLast is intentially 0 so that the constructor
  // can seamlessly pass data to submethods
  TASK_METHOD_T kLast = 0;
};

/** A task to register a Task state + Create a queue */
struct CreateTaskStateTask : public Task, SrlFlags<true, true> {
  IN hipc::ShmArchive<hipc::string> lib_name_;
  IN hipc::ShmArchive<hipc::string> state_name_;
  IN u32 queue_max_lanes_;
  IN u32 queue_num_lanes_;
  IN u32 queue_depth_;
  IN bitfield32_t queue_flags_;
  INOUT TaskStateId id_;
  TEMP int phase_;

  /** SHM default constructor */
  CreateTaskStateTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  CreateTaskStateTask(hipc::Allocator *alloc,
                      const TaskNode &task_node,
                      const DomainId &domain_id,
                      const std::string &state_name,
                      const std::string &lib_name,
                      const TaskStateId &id,
                      u32 max_lanes, u32 num_lanes,
                      u32 depth, bitfield32_t flags) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kCreateTaskState;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
    phase_ = 0;

    // Initialize
    HSHM_MAKE_AR(state_name_, alloc, state_name);
    HSHM_MAKE_AR(lib_name_, alloc, lib_name);
    id_ = id;
    queue_max_lanes_ = max_lanes;
    queue_num_lanes_ = num_lanes;
    queue_depth_ = depth;
    queue_flags_ = flags;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(lib_name_, state_name_, queue_max_lanes_, queue_num_lanes_,
       queue_depth_, queue_flags_, phase_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(id_);
  }

  TASK_SERIALIZE
};

/** A task to retrieve the ID of a task */
struct GetTaskStateIdTask : public Task, SrlFlags<true, true> {
  IN hipc::ShmArchive<hipc::string>
  state_name_;
  OUT TaskStateId
  id_;

  /** SHM default constructor */
  GetTaskStateIdTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  GetTaskStateIdTask(hipc::Allocator *alloc,
                     const TaskNode &task_node,
                     const DomainId &domain_id,
                     const std::string &state_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kGetTaskStateId;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize
    HSHM_MAKE_AR(state_name_, alloc, state_name);
  }

  ~GetTaskStateIdTask() {
    HSHM_DESTROY_AR(state_name_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(state_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(id_);
  }

  TASK_SERIALIZE
};

/** A task to destroy a Task state */
struct DestroyTaskStateTask : public Task, SrlFlags<true, true> {
  IN TaskStateId
  id_;

  /** SHM default constructor */
  DestroyTaskStateTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  DestroyTaskStateTask(hipc::Allocator *alloc,
                       const TaskNode &task_node,
                       const DomainId &domain_id,
                       const TaskStateId &id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kDestroyTaskState;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize
    id_ = id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar & id_;
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }

  TASK_SERIALIZE
};

/** A task to destroy a Task state */
struct StopRuntimeTask : public Task, SrlFlags<true, true> {
  /** SHM default constructor */
  StopRuntimeTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  StopRuntimeTask(hipc::Allocator *alloc,
                  const TaskNode &task_node,
                  const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    method_ = Method::kStopRuntime;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }

  TASK_SERIALIZE
};

/** A task to destroy a Task state */
template<int method>
struct SetWorkOrchestratorPolicyTask : public Task, SrlFlags<true, true> {
  IN TaskStateId
  policy_id_;

  /** SHM default constructor */
  SetWorkOrchestratorPolicyTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  SetWorkOrchestratorPolicyTask(hipc::Allocator *alloc,
                                const TaskNode &task_node,
                                const DomainId &domain_id,
                                const TaskStateId &policy_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = 0;
    task_state_ = LABSTOR_QM_CLIENT->admin_task_state_;
    if constexpr(method == 0)
    {
      method_ = Method::kSetWorkOrchestratorQueuePolicy;
    } else {
      method_ = Method::kSetWorkOrchestratorProcessPolicy;
    }
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Initialize
    policy_id_ = policy_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(policy_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }

  TASK_SERIALIZE
};
using SetWorkOrchestratorQueuePolicyTask = SetWorkOrchestratorPolicyTask<0>;
using SetWorkOrchestratorProcessPolicyTask = SetWorkOrchestratorPolicyTask<1>;

}  // namespace labstor::Admin

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_TASKS_H_
