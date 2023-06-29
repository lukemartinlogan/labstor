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

/** Used to define task methods */
#define TASK_METHOD_T static const u32

/** The baseline set of tasks */
struct TaskMethod {
  TASK_METHOD_T kConstruct = 0; /**< The constructor of the task */
  TASK_METHOD_T kDestruct = 1;  /**< The destructor of the task */
  TASK_METHOD_T kLast = 2;    /**< Where the next method should take place */
};

/** A generic task */
struct Task {
  bitfield32_t flags_;      /**< Properties of the task  */
  TaskExecId task_exec_;    /**< The unique name of a task executor */
  u32 method_;              /**< The method to call in the executor */
  u32 node_id_;             /**< The node that the task should run on */
  u32 hash_;                /**< Determines the lane task is keyed to */
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
