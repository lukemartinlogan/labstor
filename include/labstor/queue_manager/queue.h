//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_

#include "labstor/labstor_types.h"
#include "labstor/task_registry/task.h"
#include <vector>

/** This queue contains only latency-sensitive tasks */
#define QUEUE_LOW_LATENCY (1 << 0)
/** This queue is currently being resized */
#define QUEUE_RESIZE (1 << 1)
/** This queue is currently processing updates */
#define QUEUE_UPDATE (1 << 2)

namespace labstor {

template <typename QueueT>
class MultiQueueT;

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
