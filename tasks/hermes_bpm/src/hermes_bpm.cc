//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes_bpm/hermes_bpm.h"

namespace hermes::bpm {

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
      case Method::kPut: {
        Put(queue, reinterpret_cast<PutTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
  }

  void Put(MultiQueue *queue, PutTask *task) {
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::bpm::Server, "hermes_bpm");
