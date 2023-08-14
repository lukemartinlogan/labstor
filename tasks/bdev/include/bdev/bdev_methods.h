//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_METHODS_H_
#define LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_METHODS_H_

using labstor::TaskMethod;

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kWrite = TaskMethod::kLast;
  TASK_METHOD_T kRead = TaskMethod::kLast + 1;
  TASK_METHOD_T kAlloc = TaskMethod::kLast + 2;
  TASK_METHOD_T kFree = TaskMethod::kLast + 3;
  TASK_METHOD_T kMonitor = TaskMethod::kLast + 4;
  TASK_METHOD_T kUpdateCapacity = TaskMethod::kLast + 5;
  TASK_METHOD_T kBdevLast = TaskMethod::kLast + 6;
};

#endif  // LABSTOR_TASKS_BDEV_INCLUDE_BDEV_BDEV_TASKS_METHODS_H_
