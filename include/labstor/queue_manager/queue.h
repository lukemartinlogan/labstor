//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_

#include "labstor/labstor_types.h"
#include "labstor/task_registry/task.h"
#include <vector>

/** This queue contains only latency-sensitive tasks */
#define QUEUE_LOW_LATENCY BIT_OPT(u32, 0)
/** This queue is currently being resized */
#define QUEUE_RESIZE BIT_OPT(u32, 1)
/** This queue is currently processing updates */
#define QUEUE_UPDATE BIT_OPT(u32, 2)
/** Requests in this queue can be processed in any order */
#define QUEUE_UNORDERED BIT_OPT(u32, 3)
/** This queue is a primary queue (stores root tasks) */
#define QUEUE_PRIMARY BIT_OPT(u32, 4)

namespace labstor {

template <typename QueueT>
class MultiQueueT;

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
