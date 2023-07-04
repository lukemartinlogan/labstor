//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_

#include "labstor/labstor_types.h"

namespace labstor {

/** This task_templ reads a state */
#define TASK_READ (1 << 0)
/** This task_templ writes to a state */
#define TASK_WRITE (1 << 1)
/** This task_templ fundamentally updates a state */
#define TASK_UPDATE (1 << 2)
/** This task_templ is paused until a set of tasks complete */
#define TASK_BLOCKED (1 << 3)
/** This task_templ is latency-sensitive */
#define TASK_LOW_LATENCY (1 << 4)
/** This task_templ makes system calls and may hurt caching */
#define TASK_SYSCALL (1 << 5)
/** This task_templ does not depend on state */
#define TASK_STATELESS (1 << 6)
/** This task_templ does not depend on its position in the queue */
#define TASK_UNORDERED (1 << 7)
/** This task_templ was spawned as consequence of another task_templ */
#define TASK_INTERMEDIATE (1 << 8)
/** This task_templ is completed */
#define TASK_COMPLETE (1 << 9)

/** Used to define task_templ methods */
#define TASK_METHOD_T static const u32

/** The baseline set of tasks */
struct TaskMethod {
  TASK_METHOD_T kConstruct = 0; /**< The constructor of the task_templ */
  TASK_METHOD_T kDestruct = 1;  /**< The destructor of the task_templ */
  TASK_METHOD_T kLast = 2;    /**< Where the next method should take place */
};

/** A generic task base class */
struct Task : public hipc::ShmContainer {
  SHM_CONTAINER_TEMPLATE((Task), (Task))
  u32 key_;                 /**< Helps determine the lane task_templ is keyed to */
  TaskStateId task_state_;    /**< The unique name of a task_templ executor */
  u32 method_;              /**< The method to call in the executor */
  bitfield32_t task_flags_;    /**< Properties of the task_templ  */
  u32 node_id_;                /**< The node that the task_templ should run on */

  /**====================================
   * Task Helpers
   * ===================================*/
  
  /** Check if task_templ is complete */
  HSHM_ALWAYS_INLINE bool IsComplete() {
    return task_flags_.Any(TASK_COMPLETE);
  }

  /** Set task_templ as complete */
  HSHM_ALWAYS_INLINE void SetComplete() {
    task_flags_.SetBits(TASK_COMPLETE);
  }

  /** Wait for task_templ to complete */
  void Wait() {
    while (!IsComplete()) {
      HERMES_THREAD_MODEL->Yield();
    }
  }

  /**====================================
   * Default Constructor
   * ===================================*/

  /** Default SHM constructor */
  HSHM_ALWAYS_INLINE Task(hipc::Allocator *alloc) {
    shm_init_container(alloc);
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

/** Decorator macros */
#define IN
#define OUT
#define INOUT

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
