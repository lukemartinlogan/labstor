//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "TASK_NAME/TASK_NAME.h"

namespace labstor::TASK_NAME {

class Server : public TaskLib {
 public:
  Server() = default;

  void Construct(MultiQueue *queue, ConstructTask *task) {
    task->SetModuleComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetModuleComplete();
  }

  void Custom(MultiQueue *queue, CustomTask *task) {
    task->SetModuleComplete();
  }

 public:
#include "TASK_NAME/TASK_NAME_lib_exec.h"
};

}  // namespace labstor::TASK_NAME

LABSTOR_TASK_CC(labstor::TASK_NAME::Server, "TASK_NAME");
