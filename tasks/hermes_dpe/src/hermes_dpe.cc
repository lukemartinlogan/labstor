//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes_dpe/hermes_dpe.h"
#include "hermes_dpe/dpe_factory.h"

namespace hermes::dpe {

class Server : public TaskLib {
 public:
  mdm::Client *mdm_;

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
    AUTO_TRACE(1)
    Status result;
    std::vector<PlacementSchema> output;

    // Get the capacity/bandwidth of targets
    std::vector<TargetInfo> targets;
    // targets = mdm_->LocalGetTargetInfo();
    if (targets.size() == 0) {
      task->SetComplete();
      return;
    }
    // Calculate a placement schema
    auto *dpe =  DpeFactory::Get(task->ctx_.dpe_);
    dpe->Placement({task->data_size_}, targets, task->ctx_, output);

    // Pass result to targets
    hipc::mptr<char> data(task->data_);
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::dpe::Server, "hermes_dpe");
