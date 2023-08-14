//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_METHODS_H_
#define LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_METHODS_H_

/** The set of methods in the remote_queue task */
struct Method : public TaskMethod {
  TASK_METHOD_T kDisperse = TaskMethod::kLast;
  TASK_METHOD_T kPush = TaskMethod::kLast + 1;
  TASK_METHOD_T kAccept = TaskMethod::kLast + 2;
};

#endif  // LABSTOR_TASKS_REMOTE_QUEUE_INCLUDE_REMOTE_QUEUE_REMOTE_QUEUE_METHODS_H_
