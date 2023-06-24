//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_

#include "labstor/labstor_data_structures.h"

namespace labstor {

#define TASK_READ (1 << 0)
#define TASK_WRITE (1 << 1)
#define TASK_UPDATE (1 << 2)
#define TASK_LOW_LATENCY (1 << 3)

#define TASK_METHOD_T static const u32

struct TaskMethods {
  TASK_METHOD_T kConstruct = 0; /**< The constructor of the task */
  TASK_METHOD_T kDestruct = 1;  /**< The destructor of the task */
  TASK_METHOD_T kLast = 2;    /**< Where the next method should take place */
};

struct Task {
  TaskExecId task_exec_;      /**< The unique name of a task executor */
  u32 node_id_;               /**< The node that the task should run on */
  bitfield32_t flags_;        /**< Properties of the task  */
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_REQUEST_H_
