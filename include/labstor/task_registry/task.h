//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_

#include "labstor/labstor_types.h"
#include "labstor/network/serialize.h"

namespace labstor {

/** This task reads a state */
#define TASK_READ BIT_OPT(u32, 0)
/** This task writes to a state */
#define TASK_WRITE BIT_OPT(u32, 1)
/** This task fundamentally updates a state */
#define TASK_UPDATE BIT_OPT(u32, 2)
/** This task is paused until a set of tasks complete */
#define TASK_BLOCKED BIT_OPT(u32, 3)
/** This task is latency-sensitive */
#define TASK_LOW_LATENCY BIT_OPT(u32, 4)
/** This task makes system calls and may hurt caching */
#define TASK_SYSCALL BIT_OPT(u32, 5)
/** This task does not depend on state */
#define TASK_STATELESS BIT_OPT(u32, 6)
/** This task does not depend on its position in the queue */
#define TASK_UNORDERED BIT_OPT(u32, 7)
/** This task was spawned as consequence of another task */
#define TASK_INTERMEDIATE BIT_OPT(u32, 8)
/** This task is completed */
#define TASK_COMPLETE BIT_OPT(u32, 9)
/** This task is complete enough for the user's wait to stop */
#define TASK_USER_COMPLETE BIT_OPT(u32, 10)
/** This task was marked completed outside of the worker thread */
#define TASK_EXTERNAL_COMPLETE BIT_OPT(u32, 11)
/** This task is long-running */
#define TASK_LONG_RUNNING BIT_OPT(u32, 12)
/** This task is fire and forget. Free when completed */
#define TASK_FIRE_AND_FORGET BIT_OPT(u32, 13)
/** This task should not be run at this time */
#define TASK_DISABLE_RUN BIT_OPT(u32, 14)

/** Used to define task methods */
#define TASK_METHOD_T static inline const u32

/** The baseline set of tasks */
struct TaskMethod {
  TASK_METHOD_T kConstruct = 0; /**< The constructor of the task */
  TASK_METHOD_T kDestruct = 1;  /**< The destructor of the task */
  TASK_METHOD_T kLast = 2;    /**< Where the next method should take place */
};

/**
 * Let's say we have an I/O request to a device
 * I/O requests + MD operations need to be controlled for correctness
 * Is there a case where root tasks from different TaskStates need to be ordered? No.
 * Tasks spawned from the same root task need to be keyed to the same worker stack
 * Tasks apart of the same task group need to be ordered
 * */

/** An identifier used for representing a graph */
struct TaskNode {
  TaskId root_;         /**< The id of the root task */
  u32 node_depth_;      /**< The depth of the task in the task graph */

  /** Default constructor */
  HSHM_ALWAYS_INLINE
  TaskNode() = default;

  /** Emplace constructor for root task */
  HSHM_ALWAYS_INLINE
  TaskNode(TaskId root) {
    root_ = root;
    node_depth_ = 0;
  }

  /** Copy constructor */
  HSHM_ALWAYS_INLINE
  TaskNode(const TaskNode &other) {
    root_ = other.root_;
    node_depth_ = other.node_depth_;
  }

  /** Copy assignment operator */
  HSHM_ALWAYS_INLINE
  TaskNode& operator=(const TaskNode &other) {
    root_ = other.root_;
    node_depth_ = other.node_depth_;
    return *this;
  }

  /** Move constructor */
  HSHM_ALWAYS_INLINE
  TaskNode(TaskNode &&other) noexcept {
    root_ = other.root_;
    node_depth_ = other.node_depth_;
  }

  /** Move assignment operator */
  HSHM_ALWAYS_INLINE
  TaskNode& operator=(TaskNode &&other) noexcept {
    root_ = other.root_;
    node_depth_ = other.node_depth_;
    return *this;
  }

  /** Addition operator*/
  HSHM_ALWAYS_INLINE
  TaskNode operator+(int i) const {
    TaskNode ret;
    ret.root_ = root_;
    ret.node_depth_ = node_depth_ + i;
    return ret;
  }

  /** Null task node */
  HSHM_ALWAYS_INLINE
  static TaskNode GetNull() {
    TaskNode ret;
    ret.root_ = TaskId::GetNull();
    ret.node_depth_ = 0;
    return ret;
  }

  /** Check if null */
  HSHM_ALWAYS_INLINE
  bool IsNull() const {
    return root_.IsNull();
  }

  /** Serialization*/
  template<typename Ar>
  void serialize(Ar &ar) {
    ar(root_, node_depth_);
  }
};

/** A generic task base class */
 struct Task : public hipc::ShmContainer, public labstor::BulkSerializeable {
 SHM_CONTAINER_TEMPLATE((Task), (Task))
  TaskStateId task_state_;     /**< The unique name of a task state */
  TaskNode task_node_;         /**< The unique ID of this task in the graph */
  DomainId domain_id_;         /**< The nodes that the task should run on */
  u32 lane_hash_;              /**< Determine the lane a task is keyed to */
  u32 method_;                 /**< The method to call in the state */
  bitfield32_t task_flags_;    /**< Properties of the task */

  /**====================================
   * Task Helpers
   * ===================================*/

  /** Check if task is complete */
  HSHM_ALWAYS_INLINE bool IsComplete() {
    return task_flags_.Any(TASK_COMPLETE | TASK_USER_COMPLETE);
  }

  /** Set task as externally complete */
  HSHM_ALWAYS_INLINE void SetExternalComplete() {
    task_flags_.SetBits(TASK_EXTERNAL_COMPLETE);
  }

  /** Check if a task marked complete externally */
  HSHM_ALWAYS_INLINE bool IsExternalComplete() {
    return task_flags_.Any(TASK_EXTERNAL_COMPLETE);
  }

  /** Check if a task is fire & forget */
  HSHM_ALWAYS_INLINE bool IsFireAndForget() {
    return task_flags_.Any(TASK_FIRE_AND_FORGET);
  }

  /** Set task as complete */
  HSHM_ALWAYS_INLINE void SetComplete() {
    task_flags_.SetBits(TASK_COMPLETE);
  }

  /** Set task as user complete */
  HSHM_ALWAYS_INLINE void SetUserComplete() {
    task_flags_.SetBits(TASK_USER_COMPLETE);
  }

  /** Disable the running of a task */
  HSHM_ALWAYS_INLINE void DisableRun() {
    task_flags_.SetBits(TASK_DISABLE_RUN);
  }

  /** Check if running task is disable */
  HSHM_ALWAYS_INLINE bool IsRunDisabled() {
    return task_flags_.Any(TASK_DISABLE_RUN);
  }

  /** Wait for task to complete */
  void Wait() {
    while (!IsComplete()) {
      for (int i = 0; i < 100000; ++i) {
        if (IsComplete()) {
          return;
        }
      }
      HERMES_THREAD_MODEL->Yield();
    }
  }

  /**====================================
   * Default Constructor
   * ===================================*/

  /** Default SHM constructor */
  HSHM_ALWAYS_INLINE explicit
  Task(hipc::Allocator *alloc) {
    shm_init_container(alloc);
  }

  /** SHM constructor */
  HSHM_ALWAYS_INLINE explicit
  Task(hipc::Allocator *alloc,
       const TaskNode &task_node) {
    shm_init_container(alloc);
    task_node_ = task_node;
  }

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  Task(hipc::Allocator *alloc,
       const TaskNode &task_node,
       const DomainId &domain_id,
       const TaskStateId &task_state,
       u32 lane_hash,
       u32 method,
       bitfield32_t task_flags) {
    shm_init_container(alloc);
    task_node_ = task_node;
    lane_hash_ = lane_hash;
    task_state_ = task_state;
    method_ = method;
    domain_id_ = domain_id;
    task_flags_ = task_flags;
  }

  /**====================================
   * Copy Constructors
   * ===================================*/

  /** SHM copy constructor */
  HSHM_ALWAYS_INLINE explicit Task(hipc::Allocator *alloc, const Task &other) {}

  /** SHM copy assignment operator */
  HSHM_ALWAYS_INLINE Task& operator=(const Task &other) {
    return *this;
  }

  /**====================================
   * Move Constructors
   * ===================================*/

  /** SHM move constructor. */
  HSHM_ALWAYS_INLINE Task(hipc::Allocator *alloc, Task &&other) noexcept {}

  /** SHM move assignment operator. */
  HSHM_ALWAYS_INLINE Task& operator=(Task &&other) noexcept {
    return *this;
  }

  /**====================================
   * Destructor
   * ===================================*/

  /** SHM destructor.  */
  HSHM_ALWAYS_INLINE void shm_destroy_main() {}

  /** Check if the Task is empty */
  HSHM_ALWAYS_INLINE bool IsNull() const { return false; }

  /** Sets this Task as empty */
  HSHM_ALWAYS_INLINE void SetNull() {}

 /**====================================
  * Serialization
  * ===================================*/
 template<typename Ar>
 void task_serialize(Ar &ar) {
   ar(task_state_, task_node_, domain_id_, lane_hash_, method_, task_flags_);
 }
};

/** A task is NOT compatible with shared memory */
typedef Task LocalTask;

/** A task IS compatible with shared memory */
typedef Task IpcTask;

/** Decorator macros */
#define IN
#define OUT
#define INOUT
#define TEMP

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
