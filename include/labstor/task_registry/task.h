//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_

#include "labstor/labstor_types.h"

namespace labstor {

/** This task reads a state */
#define TASK_READ (1 << 0)
/** This task writes to a state */
#define TASK_WRITE (1 << 1)
/** This task fundamentally updates a state */
#define TASK_UPDATE (1 << 2)
/** This task is paused until a set of tasks complete */
#define TASK_BLOCKED (1 << 3)
/** This task is latency-sensitive */
#define TASK_LOW_LATENCY (1 << 4)
/** This task makes system calls and may hurt caching */
#define TASK_SYSCALL (1 << 5)
/** This task does not depend on state */
#define TASK_STATELESS (1 << 6)
/** This task does not depend on its position in the queue */
#define TASK_UNORDERED (1 << 7)
/** This task was spawned as consequence of another task */
#define TASK_INTERMEDIATE (1 << 8)
/** This task is completed */
#define TASK_COMPLETE (1 << 9)
/** This task was marked completed outside of the worker thread */
#define TASK_EXTERNAL_COMPLETE (1 << 10)
/** This task is long-running */
#define TASK_LONG_RUNNING (1 << 11)

/** Used to define task methods */
#define TASK_METHOD_T static inline const u32

/** The baseline set of tasks */
struct TaskMethod {
  TASK_METHOD_T kConstruct = 0; /**< The constructor of the task */
  TASK_METHOD_T kDestruct = 1;  /**< The destructor of the task */
  TASK_METHOD_T kLast = 2;    /**< Where the next method should take place */
};

/** A generic task base class */
struct Task : public hipc::ShmContainer {
 SHM_CONTAINER_TEMPLATE((Task), (Task))
  u32 key_;                    /**< Helps determine the lane task is keyed to */
  TaskStateId task_state_;     /**< The unique name of a task state */
  u32 method_;                 /**< The method to call in the state */
  bitfield32_t task_flags_;    /**< Properties of the task  */
  DomainId domain_id_;         /**< The node that the task should run on */

  /**====================================
   * Task Helpers
   * ===================================*/

  /** Check if task is complete */
  HSHM_ALWAYS_INLINE bool IsComplete() {
    return task_flags_.Any(TASK_COMPLETE);
  }

  /** Set task as externally complete */
  HSHM_ALWAYS_INLINE void SetExternalComplete() {
    task_flags_.SetBits(TASK_EXTERNAL_COMPLETE);
  }

  /** Check if a task marked complete externally */
  HSHM_ALWAYS_INLINE bool IsExternalComplete() {
    return task_flags_.Any(TASK_EXTERNAL_COMPLETE);
  }

  /** Set task as complete */
  HSHM_ALWAYS_INLINE void SetComplete() {
    task_flags_.SetBits(TASK_COMPLETE);
  }

  /** Wait for task to complete */
  void Wait() {
    while (!IsComplete()) {
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

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  Task(hipc::Allocator *alloc, u32 key,
       TaskStateId task_state,
       u32 method, const DomainId &domain_id,
       bitfield32_t task_flags) {
    shm_init_container(alloc);
    key_ = key;
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
};

/** A task is NOT compatible with shared memory */
typedef Task LocalTask;

/** A task IS compatible with shared memory */
typedef Task IpcTask;

/** Decorator macros */
#define IN
#define OUT
#define INOUT

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
