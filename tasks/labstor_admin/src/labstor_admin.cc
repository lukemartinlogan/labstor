//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor/api/labstor_runtime.h"

namespace labstor {

class Admin : public TaskLib {
  void Run(MultiQueue *queue, u32 method, Task *task) override {
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::Admin, "admin");
