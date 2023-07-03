//
// Created by llogan on 7/2/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_SCHEDULER_H_
#define LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_SCHEDULER_H_

#include "labstor/task_registry/task.h"

namespace labstor {

/** The set of methods in the admin task */
struct SchedulerMethod : public TaskMethod {
  TASK_METHOD_T kSchedule = TaskMethod::kLast;
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_WORK_ORCHESTRATOR_SCHEDULER_H_
