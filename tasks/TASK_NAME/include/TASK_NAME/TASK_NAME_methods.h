//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_METHODS_H_
#define LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_METHODS_H_

#include "labstor/api/labstor_client.h"

namespace labstor::TASK_NAME {

/** The set of methods in the TASK_NAME task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = TaskMethod::kLast;
};

} // namespace labstor::TASK_NAME

#endif  // LABSTOR_TASKS_TASK_TEMPL_INCLUDE_TASK_NAME_TASK_NAME_METHODS_H_
