//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_METHODS_H_
#define LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_METHODS_H_

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

#endif  // LABSTOR_TASKS_LABSTOR_ADMIN_INCLUDE_LABSTOR_ADMIN_LABSTOR_ADMIN_METHODS_H_
