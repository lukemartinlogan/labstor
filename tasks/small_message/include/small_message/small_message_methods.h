//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_SMALL_MESSAGE_INCLUDE_SMALL_MESSAGE_SMALL_MESSAGE_METHODS_H_
#define LABSTOR_TASKS_SMALL_MESSAGE_INCLUDE_SMALL_MESSAGE_SMALL_MESSAGE_METHODS_H_

#include "labstor/api/labstor_client.h"

namespace labstor::small_message {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kMd = TaskMethod::kLast;
  TASK_METHOD_T kIo = TaskMethod::kLast + 1;
};

}  // namespace labstor::small_message

#endif  // LABSTOR_TASKS_SMALL_MESSAGE_INCLUDE_SMALL_MESSAGE_SMALL_MESSAGE_METHODS_H_
