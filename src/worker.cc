//
// Created by lukemartinlogan on 6/27/23.
//

#include "labstor/api/labstor_runtime.h"
#include "labstor/work_orchestrator/worker.h"
#include "labstor/work_orchestrator/work_orchestrator.h"

namespace labstor {

// Should queues work across task_templ executors?
// Should queues be specific to task_templ executors?

void Worker::Loop() {
  while (LABSTOR_WORK_ORCHESTRATOR->IsAlive()) {
    Run();
  }
  Run();
}

void Worker::Run() {
  // Iterate over worker queue
  // Get TaskExecutor instance from TaskRegistry
    // Dequeue requests for this executor
  // Get next executor
}

}  // namespace labstor