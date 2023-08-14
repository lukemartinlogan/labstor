#ifndef LABSTOR_LABSTOR_ADMIN_METHODS_H_
#define LABSTOR_LABSTOR_ADMIN_METHODS_H_

using labstor::TaskMethod;
using labstor::BinaryOutputArchive;
using labstor::BinaryInputArchive;
using labstor::Task;

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kRegisterTaskLib = kLast + 0;
  TASK_METHOD_T kDestroyTaskLib = kLast + 1;
  TASK_METHOD_T kCreateTaskState = kLast + 2;
  TASK_METHOD_T kGetTaskStateId = kLast + 3;
  TASK_METHOD_T kDestroyTaskState = kLast + 4;
  TASK_METHOD_T kStopRuntime = kLast + 5;
  TASK_METHOD_T kSetWorkOrchestratorQueuePolicy = kLast + 6;
  TASK_METHOD_T kSetWorkOrchestratorProcessPolicy = kLast + 7;
};

#endif  // LABSTOR_LABSTOR_ADMIN_METHODS_H_