//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes_bpm/hermes_bpm.h"

namespace labstor::hermes_bpm {

class Server : public TaskLib {
 public:
  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kCustom: {
        Custom(queue, reinterpret_cast<CustomTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
  }

  void Custom(MultiQueue *queue, CustomTask *task) {
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::hermes_bpm::Server, "hermes_bpm");
